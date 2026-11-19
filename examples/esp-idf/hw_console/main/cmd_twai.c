#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "cmd_twai.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "twai";

/* Board CAN transceiver pins (TWAI_TX / TWAI_RX in the Arduino variant) */
#define TWAI_TX_GPIO        22
#define TWAI_RX_GPIO        21
#define TWAI_DEFAULT_BPS    500000
#define TWAI_RXQ_LEN        32
#define TWAI_TX_TIMEOUT_MS  1000
#define TWAI_MAX_DATA       8

typedef struct {
    twai_frame_header_t header;
    uint8_t data[TWAI_MAX_DATA];
} rx_item_t;

static twai_node_handle_t s_node;
static QueueHandle_t s_rxq;
static twai_onchip_node_config_t s_cfg;
static volatile uint32_t s_rx_count;
static volatile uint32_t s_tx_ok;
static volatile uint32_t s_tx_fail;

static const char *state_name(twai_error_state_t state)
{
    switch (state) {
    case TWAI_ERROR_ACTIVE:  return "error-active";
    case TWAI_ERROR_WARNING: return "error-warning";
    case TWAI_ERROR_PASSIVE: return "error-passive";
    case TWAI_ERROR_BUS_OFF: return "bus-off";
    default:                 return "?";
    }
}

/* Every received frame is drained here (so the hardware never stalls) and
 * queued for twai_dump; frames are dropped while the queue is full. */
static bool on_rx_done(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    rx_item_t item = {0};
    twai_frame_t frame = { .buffer = item.data, .buffer_len = sizeof(item.data) };
    BaseType_t woken = pdFALSE;
    if (twai_node_receive_from_isr(handle, &frame) == ESP_OK) {
        item.header = frame.header;
        s_rx_count++;
        xQueueSendFromISR(s_rxq, &item, &woken);
    }
    return woken == pdTRUE;
}

static bool on_tx_done(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    if (edata->is_tx_success) {
        s_tx_ok++;
    } else {
        s_tx_fail++;
    }
    return false;
}

/* Only one controller is wired to the transceiver */
static bool check_controller(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "twai0") != 0) {
        printf("controller must be twai0\n");
        return false;
    }
    return true;
}

static bool check_running(void)
{
    if (!s_node) {
        printf("twai0 not initialized (use: twai_init twai0)\n");
        return false;
    }
    return true;
}

/* twai_init twai0 [-t <gpio>] [-r <gpio>] [-b <bps>] [--loopback] [--self-test] [--listen] */
static int cmd_twai_init(int argc, char **argv)
{
    if (!check_controller(argc, argv)) {
        return 1;
    }
    if (s_node) {
        printf("twai0 already running, use twai_deinit twai0 first\n");
        return 1;
    }
    twai_onchip_node_config_t cfg = {
        .io_cfg = {
            .tx = TWAI_TX_GPIO,
            .rx = TWAI_RX_GPIO,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },
        .bit_timing = { .bitrate = TWAI_DEFAULT_BPS },
        .fail_retry_cnt = 0,     /* report a missing ACK instead of retrying forever */
        .tx_queue_depth = 5,
    };
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            cfg.io_cfg.tx = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            cfg.io_cfg.rx = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            cfg.bit_timing.bitrate = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--loopback") == 0) {
            cfg.flags.enable_loopback = 1;
        } else if (strcmp(argv[i], "--self-test") == 0) {
            cfg.flags.enable_self_test = 1;
        } else if (strcmp(argv[i], "--listen") == 0) {
            cfg.flags.enable_listen_only = 1;
        } else {
            printf("unknown option %s\n", argv[i]);
            return 1;
        }
    }
    if (!s_rxq) {
        s_rxq = xQueueCreate(TWAI_RXQ_LEN, sizeof(rx_item_t));
        if (!s_rxq) {
            return 1;
        }
    }
    xQueueReset(s_rxq);
    s_rx_count = s_tx_ok = s_tx_fail = 0;

    const twai_event_callbacks_t cbs = {
        .on_rx_done = on_rx_done,
        .on_tx_done = on_tx_done,
    };
    esp_err_t err = twai_new_node_onchip(&cfg, &s_node);
    if (err == ESP_OK) {
        err = twai_node_register_event_callbacks(s_node, &cbs, NULL);
    }
    if (err == ESP_OK) {
        err = twai_node_enable(s_node);
    }
    if (err != ESP_OK) {
        printf("twai_init failed: %s\n", esp_err_to_name(err));
        if (s_node) {
            twai_node_delete(s_node);
            s_node = NULL;
        }
        return 1;
    }
    s_cfg = cfg;
    printf("twai0 up: %lu bps, TX GPIO%d, RX GPIO%d%s%s%s\n", (unsigned long)cfg.bit_timing.bitrate,
           cfg.io_cfg.tx, cfg.io_cfg.rx,
           cfg.flags.enable_loopback ? ", loopback" : "",
           cfg.flags.enable_self_test ? ", self-test" : "",
           cfg.flags.enable_listen_only ? ", listen-only" : "");
    return 0;
}

static void dump_stop(void)
{
    int id = job_find("twai_dump");
    if (id > 0) {
        job_kill(id);
    }
}

static int cmd_twai_deinit(int argc, char **argv)
{
    if (!check_controller(argc, argv) || !check_running()) {
        return 1;
    }
    dump_stop();
    twai_node_disable(s_node);
    esp_err_t err = twai_node_delete(s_node);
    s_node = NULL;
    printf("twai0 down\n");
    return err == ESP_OK ? 0 : 1;
}

/* <id>#<hex data> or <id>#R; an id of more than 3 hex digits is a 29-bit extended id */
static bool parse_frame(const char *s, twai_frame_t *frame, uint8_t *data)
{
    const char *hash = strchr(s, '#');
    if (!hash || hash == s || hash - s > 8) {
        return false;
    }
    char *end = NULL;
    uint32_t id = strtoul(s, &end, 16);
    if (end != hash) {
        return false;
    }
    bool ext = hash - s > 3;
    if (id > (ext ? 0x1FFFFFFFu : 0x7FFu)) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->header.id = id;
    frame->header.ide = ext;
    frame->buffer = data;
    const char *p = hash + 1;
    if (*p == 'R' || *p == 'r') {
        frame->header.rtr = 1;
        return true;
    }
    size_t len = strlen(p);
    if (len % 2 || len / 2 > TWAI_MAX_DATA) {
        return false;
    }
    for (size_t i = 0; i < len / 2; i++) {
        char byte[3] = { p[2 * i], p[2 * i + 1], 0 };
        data[i] = (uint8_t)strtoul(byte, &end, 16);
        if (*end) {
            return false;
        }
    }
    frame->header.dlc = len / 2;
    frame->buffer_len = len / 2;
    return true;
}

/* twai_send twai0 123#DEADBEEF */
static int cmd_twai_send(int argc, char **argv)
{
    if (!check_controller(argc, argv) || !check_running()) {
        return 1;
    }
    twai_frame_t frame;
    uint8_t data[TWAI_MAX_DATA];
    if (argc < 3 || !parse_frame(argv[2], &frame, data)) {
        printf("usage: twai_send twai0 <id>#<hex data>   e.g. 123#DEADBEEF, 12345678#0011, 123#R\n");
        return 1;
    }
    uint32_t ok_before = s_tx_ok;
    esp_err_t err = twai_node_transmit(s_node, &frame, TWAI_TX_TIMEOUT_MS);
    if (err == ESP_OK) {
        err = twai_node_transmit_wait_all_done(s_node, TWAI_TX_TIMEOUT_MS);
    }
    bool pass = err == ESP_OK && s_tx_ok != ok_before;
    if (pass) {
        printf("sent %s\n", argv[2]);
    } else {
        printf("send failed: %s (no ACK? needs a second node, or twai_init with --self-test)\n",
               err == ESP_OK ? "bus error" : esp_err_to_name(err));
    }
    report_set("can.tx", pass ? REPORT_PASS : REPORT_FAIL, NULL, pass ? NULL : "no ACK / bus error");
    return pass ? 0 : 1;
}

/* ---- twai_dump: background job printing received frames like candump ---- */
static bool dump_iter(void *ctx)
{
    rx_item_t item;
    if (xQueueReceive(s_rxq, &item, pdMS_TO_TICKS(100)) != pdTRUE) {
        return true;
    }
    if (item.header.ide) {
        printf("  twai0  %08lX  ", (unsigned long)item.header.id);
    } else {
        printf("  twai0  %03lX  ", (unsigned long)item.header.id);
    }
    if (item.header.rtr) {
        printf("[%u]  remote request\n", item.header.dlc);
    } else {
        int len = item.header.dlc < TWAI_MAX_DATA ? item.header.dlc : TWAI_MAX_DATA;
        printf("[%d] ", len);
        for (int i = 0; i < len; i++) {
            printf(" %02X", item.data[i]);
        }
        printf("\n");
    }
    job_report(true, 0);
    report_set("can.rx", REPORT_PASS, NULL, NULL);
    return true;
}

/* twai_dump twai0 | twai_dump twai0 --stop */
static int cmd_twai_dump(int argc, char **argv)
{
    if (!check_controller(argc, argv)) {
        return 1;
    }
    if (argc > 2 && strcmp(argv[2], "--stop") == 0) {
        dump_stop();
        return 0;
    }
    if (!check_running()) {
        return 1;
    }
    if (job_find("twai_dump") > 0) {
        printf("twai_dump already running\n");
        return 1;
    }
    job_desc_t d = {
        .name = "twai_dump",
        .iter = dump_iter,
        .stack_size = 4096,
        .res = RES_MAX,
    };
    if (job_start(&d) < 0) {
        return 1;
    }
    printf("dumping twai0; stop with: twai_dump twai0 --stop\n");
    return 0;
}

static int cmd_twai_info(int argc, char **argv)
{
    if (!check_controller(argc, argv) || !check_running()) {
        return 1;
    }
    twai_node_status_t st;
    twai_node_record_t rec;
    ESP_RETURN_ON_ERROR(twai_node_get_info(s_node, &st, &rec), TAG, "get info");
    printf("twai0: %lu bps, TX GPIO%d, RX GPIO%d, mode:%s%s%s%s\n",
           (unsigned long)s_cfg.bit_timing.bitrate, s_cfg.io_cfg.tx, s_cfg.io_cfg.rx,
           s_cfg.flags.enable_loopback ? " loopback" : "",
           s_cfg.flags.enable_self_test ? " self-test" : "",
           s_cfg.flags.enable_listen_only ? " listen-only" : "",
           s_cfg.flags.enable_loopback || s_cfg.flags.enable_self_test || s_cfg.flags.enable_listen_only ? "" : " normal");
    printf("  state %s, TEC %u, REC %u, bus errors %lu\n", state_name(st.state),
           st.tx_error_count, st.rx_error_count, (unsigned long)rec.bus_err_num);
    printf("  tx ok %lu, tx failed %lu, rx %lu\n",
           (unsigned long)s_tx_ok, (unsigned long)s_tx_fail, (unsigned long)s_rx_count);
    return 0;
}

static int cmd_twai_recover(int argc, char **argv)
{
    if (!check_controller(argc, argv) || !check_running()) {
        return 1;
    }
    esp_err_t err = twai_node_recover(s_node);
    printf("recover: %s\n", esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

void register_twai_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "twai_init",    .help = "Start CAN. twai_init twai0 [-b 500000] [-t 22] [-r 21] [--self-test] [--loopback] [--listen]", .func = cmd_twai_init },
        { .command = "twai_deinit",  .help = "Stop CAN. twai_deinit twai0", .func = cmd_twai_deinit },
        { .command = "twai_send",    .help = "Send a frame. twai_send twai0 123#DEADBEEF | 12345678#00 | 123#R", .func = cmd_twai_send },
        { .command = "twai_dump",    .help = "Print received frames in the background. twai_dump twai0 | twai_dump twai0 --stop", .func = cmd_twai_dump },
        { .command = "twai_info",    .help = "Show CAN state and counters. twai_info twai0", .func = cmd_twai_info },
        { .command = "twai_recover", .help = "Recover from bus-off. twai_recover twai0", .func = cmd_twai_recover },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
