#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_console.h"
#include "cmd_serial.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "serial";

#define SERIAL_DEFAULT_BAUD  115200
#define SERIAL_BUF_SIZE      1024
#define ECHO_CHUNK           128

/* Pins as in examples/esp-idf/06_uart */
typedef struct {
    const char *name;
    uart_port_t port;
    int tx;
    int rx;
    int rts;                /* RS485 DE/RE, driven by the UART in half-duplex mode */
    uart_mode_t mode;
    res_id_t res;
} serial_port_t;

static const serial_port_t s_ports[] = {
    { "rs232", UART_NUM_2, 28, 29, UART_PIN_NO_CHANGE, UART_MODE_UART, RES_RS232 },
    { "rs485", UART_NUM_1, 26, 27, 34, UART_MODE_RS485_HALF_DUPLEX, RES_RS485 },
};

static esp_err_t serial_open(const serial_port_t *p, int baud)
{
    if (p->rts >= 0) {
        /* Keep the transceiver in receive mode until the UART takes over DE/RE */
        gpio_set_direction(p->rts, GPIO_MODE_OUTPUT);
        gpio_set_level(p->rts, 0);
    }
    const uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(p->port, SERIAL_BUF_SIZE, SERIAL_BUF_SIZE, 0, NULL, 0), TAG, "install");
    esp_err_t err = uart_param_config(p->port, &cfg);
    if (err == ESP_OK) {
        err = uart_set_pin(p->port, p->tx, p->rx, p->rts, UART_PIN_NO_CHANGE);
    }
    if (err == ESP_OK) {
        err = uart_set_mode(p->port, p->mode);
    }
    if (err != ESP_OK) {
        uart_driver_delete(p->port);
        ESP_LOGE(TAG, "%s config: %s", p->name, esp_err_to_name(err));
        return err;
    }
    uart_flush_input(p->port);
    return ESP_OK;
}

static void serial_close(const serial_port_t *p)
{
    uart_driver_delete(p->port);
}

/* Print received bytes, non-printable ones as '.' */
static void print_bytes(const char *prefix, const uint8_t *data, int len)
{
    printf("%s %d bytes: ", prefix, len);
    for (int i = 0; i < len; i++) {
        putchar(isprint(data[i]) ? data[i] : '.');
    }
    putchar('\n');
}

static int serial_send(const serial_port_t *p, int baud, const char *text)
{
    /* While "<port> echo" runs it owns the open UART: write through it instead
     * of reopening (uart_write_bytes is thread-safe), at the echo's baud rate. */
    char echo_name[16];
    snprintf(echo_name, sizeof(echo_name), "%s echo", p->name);
    const char *owner = res_owner(p->res);
    bool shared = owner && strcmp(owner, echo_name) == 0 && uart_is_driver_installed(p->port);
    if (shared) {
        uint32_t cur = 0;
        uart_get_baudrate(p->port, &cur);
        baud = (int)cur;
    } else {
        if (!res_lock(p->res, p->name)) {
            return 1;
        }
        if (serial_open(p, baud) != ESP_OK) {
            res_unlock(p->res);
            return 1;
        }
    }
    uart_write_bytes(p->port, text, strlen(text));
    uart_write_bytes(p->port, "\r\n", 2);
    esp_err_t err = uart_wait_tx_done(p->port, pdMS_TO_TICKS(1000));
    if (!shared) {
        serial_close(p);
        res_unlock(p->res);
    }
    printf("%s sent %u bytes at %d baud%s\n", p->name, (unsigned)strlen(text) + 2, baud,
           err == ESP_OK ? "" : " (tx timeout)");
    char item[24];
    snprintf(item, sizeof(item), "%s.tx", p->name);
    report_set(item, REPORT_MANUAL, NULL, "check text on the PC terminal");
    return err == ESP_OK ? 0 : 1;
}

/* ---- echo: background job, prints and returns everything received ---- */
typedef struct {
    serial_port_t port;      /* copy: -t / -r may override the pins */
    int baud;
    bool opened;
    bool got_data;
    char name[16];
    uint8_t buf[ECHO_CHUNK];
} echo_ctx_t;

static bool echo_iter(void *arg)
{
    echo_ctx_t *e = arg;
    const serial_port_t *p = &e->port;
    if (!e->opened) {
        if (serial_open(p, e->baud) != ESP_OK) {
            return false;
        }
        e->opened = true;
    }
    int n = uart_read_bytes(p->port, e->buf, sizeof(e->buf), pdMS_TO_TICKS(100));
    if (n > 0) {
        print_bytes(p->name, e->buf, n);
        int written = uart_write_bytes(p->port, e->buf, n);
        esp_err_t err = uart_wait_tx_done(p->port, pdMS_TO_TICKS(1000));
        printf("%s echoed %d/%d bytes%s\n", p->name, written, n, err == ESP_OK ? "" : " (tx timeout)");
        job_report(written == n && err == ESP_OK, n);
        if (!e->got_data) {
            e->got_data = true;
            char item[24];
            snprintf(item, sizeof(item), "%s.rx", p->name);
            report_set(item, REPORT_PASS, NULL, "data received, check echo on the PC");
        }
    }
    return true;
}

static void echo_cleanup(void *arg)
{
    echo_ctx_t *e = arg;
    if (e->opened) {
        serial_close(&e->port);
    }
    free(e);
}

static int serial_echo(const serial_port_t *p, int baud)
{
    echo_ctx_t *e = calloc(1, sizeof(*e));
    if (!e) {
        return 1;
    }
    e->port = *p;
    e->baud = baud;
    snprintf(e->name, sizeof(e->name), "%s echo", p->name);
    job_desc_t d = {
        .name = e->name,
        .iter = echo_iter,
        .cleanup = echo_cleanup,
        .ctx = e,
        .stack_size = 4096,
        .res = p->res,
    };
    if (job_start(&d) < 0) {
        free(e);
        return 1;
    }
    printf("%s echo at %d baud: data sent from the PC is printed and sent back; stop with kill <id>\n",
           p->name, baud);
    return 0;
}

/* rs232|rs485 send <text...>|echo [-b <baud>] [-t <tx gpio>] [-r <rx gpio>] */
static int cmd_serial(int argc, char **argv)
{
    serial_port_t port = strcmp(argv[0], "rs485") == 0 ? s_ports[1] : s_ports[0];
    const serial_port_t *p = &port;
    int baud = SERIAL_DEFAULT_BAUD;
    char text[128] = "";
    const char *action = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            baud = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            port.tx = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            port.rx = atoi(argv[++i]);
        } else if (!action) {
            action = argv[i];
        } else {
            if (text[0]) {
                strlcat(text, " ", sizeof(text));
            }
            strlcat(text, argv[i], sizeof(text));
        }
    }
    if (action && baud > 0) {
        if (strcmp(action, "send") == 0 && text[0]) {
            return serial_send(p, baud, text);
        }
        if (strcmp(action, "echo") == 0) {
            return serial_echo(p, baud);
        }
    }
    printf("usage: %s send <text> | echo [-b <baud>] [-t <tx gpio>] [-r <rx gpio>]\n", p->name);
    return 1;
}

void register_serial_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "rs232", .help = "RS232 (UART2, TX GPIO28 / RX GPIO29). rs232 send <text> | echo [-b 115200]", .func = cmd_serial },
        { .command = "rs485", .help = "RS485 (UART1, TX GPIO26 / RX GPIO27 / DE GPIO34). rs485 send <text> | echo [-b 115200]", .func = cmd_serial },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
