#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "dev_ledc_ctrl.h"
#include "dev_display_lcd.h"
#include "dev_lcd_touch.h"
#include "esp_board_periph.h"
#include "periph_ledc.h"
#include "driver/ledc.h"
#include "cmd_display.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "display";
#define STRIP_LINES 32

static esp_lcd_panel_handle_t s_panel;
static uint16_t s_w;
static uint16_t s_h;
static void *s_brightness;
static esp_lcd_touch_handle_t s_touch;

static esp_err_t backlight_set_percent(int percent)
{
    dev_ledc_ctrl_config_t *device_cfg = NULL;
    periph_ledc_config_t *ledc_cfg = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_config("lcd_brightness", (void **)&device_cfg),
                        TAG, "brightness config");
    ESP_RETURN_ON_ERROR(esp_board_periph_get_config(device_cfg->ledc_name, (void **)&ledc_cfg),
                        TAG, "LEDC config");

    periph_ledc_handle_t *ledc = (periph_ledc_handle_t *)s_brightness;
    uint32_t max_duty = (1U << ledc_cfg->duty_resolution) - 1U;
    uint32_t duty = ((uint32_t)percent * max_duty) / 100U;
    ESP_RETURN_ON_ERROR(ledc_set_duty(ledc->speed_mode, ledc->channel, duty), TAG, "set backlight duty");
    return ledc_update_duty(ledc->speed_mode, ledc->channel);
}

/* Initialise only the backlight device (enable GPIO + LEDC); the panel is not touched. */
static esp_err_t backlight_ensure(void)
{
    if (!s_brightness) {
        ESP_RETURN_ON_ERROR(esp_board_device_init("lcd_brightness"), TAG, "lcd_brightness");
        ESP_RETURN_ON_ERROR(esp_board_device_get_handle("lcd_brightness", &s_brightness), TAG, "brightness handle");
    }
    return ESP_OK;
}

esp_err_t display_ensure(esp_lcd_panel_handle_t *panel, uint16_t *width, uint16_t *height)
{
    if (!s_panel) {
        /* Enable and configure the backlight driver before panel initialization. */
        if (backlight_ensure() != ESP_OK) {
            printf("brightness device unavailable\n");
        }

        ESP_RETURN_ON_ERROR(esp_board_device_init("display_lcd"), TAG, "display_lcd");
        dev_display_lcd_handles_t *h = NULL;
        ESP_RETURN_ON_ERROR(esp_board_device_get_handle("display_lcd", (void **)&h), TAG, "handle");
        dev_display_lcd_config_t *cfg = NULL;
        ESP_RETURN_ON_ERROR(esp_board_device_get_config("display_lcd", (void **)&cfg), TAG, "config");
        s_panel = h->panel_handle;
        s_w = cfg->lcd_width;
        s_h = cfg->lcd_height;
        printf("lcd %ux%u ready\n", s_w, s_h);
    }
    if (panel) {
        *panel = s_panel;
    }
    if (width) {
        *width = s_w;
    }
    if (height) {
        *height = s_h;
    }
    return ESP_OK;
}

/* RGB565 */
static uint16_t pattern_pixel(const char *pattern, int x, int y)
{
    static const uint16_t bars[8] = { 0xFFFF, 0xFFE0, 0x07FF, 0x07E0, 0xF81F, 0xF800, 0x001F, 0x0000 };
    if (strcmp(pattern, "colorbar") == 0) {
        return bars[(x * 8) / s_w];
    }
    if (strcmp(pattern, "checker") == 0) {
        return (((x / 32) + (y / 32)) & 1) ? 0xFFFF : 0x0000;
    }
    if (strcmp(pattern, "red") == 0) {
        return 0xF800;
    }
    if (strcmp(pattern, "green") == 0) {
        return 0x07E0;
    }
    if (strcmp(pattern, "blue") == 0) {
        return 0x001F;
    }
    if (strcmp(pattern, "white") == 0) {
        return 0xFFFF;
    }
    return 0x0000; /* black */
}

/* fbtest colorbar|red|green|blue|white|black|checker */
static int cmd_fbtest(int argc, char **argv)
{
    const char *pattern = argc > 1 ? argv[1] : "colorbar";
    if (display_ensure(NULL, NULL, NULL) != ESP_OK) {
        return 1;
    }
    if (!res_lock(RES_LCD, "fbtest")) {
        return 1;
    }
    size_t strip_bytes = (size_t)s_w * STRIP_LINES * 2;
    uint16_t *strip = heap_caps_malloc(strip_bytes, MALLOC_CAP_SPIRAM);
    if (!strip) {
        res_unlock(RES_LCD);
        return 1;
    }
    for (int y0 = 0; y0 < s_h; y0 += STRIP_LINES) {
        int lines = (y0 + STRIP_LINES <= s_h) ? STRIP_LINES : s_h - y0;
        for (int y = 0; y < lines; y++) {
            for (int x = 0; x < s_w; x++) {
                strip[y * s_w + x] = pattern_pixel(pattern, x, y0 + y);
            }
        }
        esp_lcd_panel_draw_bitmap(s_panel, 0, y0, s_w, y0 + lines, strip);
    }
    free(strip);
    res_unlock(RES_LCD);
    char note[48];
    snprintf(note, sizeof(note), "%s shown, check visually", pattern);
    report_set("lcd.pattern", REPORT_MANUAL, NULL, note);
    return 0;
}

static int cmd_backlight(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: backlight <0-100|on|off>\n");
        return 1;
    }
    bool on_off = strcmp(argv[1], "on") == 0 || strcmp(argv[1], "off") == 0;
    if (on_off) {
        /* on/off only drives the backlight enable GPIO; do not initialise the panel. */
        if (backlight_ensure() != ESP_OK) {
            printf("brightness device unavailable\n");
            return 1;
        }
    } else if (display_ensure(NULL, NULL, NULL) != ESP_OK || !s_brightness) {
        printf("brightness device unavailable\n");
        return 1;
    }
    if (on_off) {
        esp_err_t err = esp_board_device_power_ctrl("lcd_brightness", strcmp(argv[1], "on") == 0);
        if (err != ESP_OK) {
            printf("failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        report_set("lcd.backlight", REPORT_MANUAL, NULL, "check backlight enable GPIO");
        return 0;
    }

    char *end = NULL;
    int pct = (int)strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || pct < 0 || pct > 100) {
        printf("usage: backlight <0-100|on|off>\n");
        return 1;
    }
    esp_err_t err = backlight_set_percent(pct);
    if (err != ESP_OK) {
        printf("failed: %s\n", esp_err_to_name(err));
        return 1;
    }
    report_set("lcd.backlight", REPORT_MANUAL, NULL, "check brightness change");
    return 0;
}

/* ---- evtest: drawing board on the panel until killed ----
 * The screen is cleared and five targets (four corners + centre) are drawn in
 * red. Every finger paints its own colour; a target turns green once touched,
 * and lcd.touch becomes PASS when all five are hit. */
#define EV_MAX_POINTS    5
#define EV_BRUSH         8      /* brush square size, px */
#define EV_TARGET_INSET  40     /* target centre distance from the panel edge, px */
#define EV_TARGET_ARM    20     /* crosshair half-length, px */
#define EV_TARGET_HIT    40     /* a touch this close to a target centre hits it, px */
#define EV_TARGETS       5
#define RGB565_RED       0xF800
#define RGB565_GREEN     0x07E0

static const uint16_t s_brush_colors[EV_MAX_POINTS] = { 0xFFE0, 0x07FF, 0xF81F, 0xFD20, 0xFFFF };

typedef struct {
    int tx[EV_TARGETS];
    int ty[EV_TARGETS];
    bool hit[EV_TARGETS];
    int hits;
    bool down[EV_MAX_POINTS];
    int last_x[EV_MAX_POINTS];
    int last_y[EV_MAX_POINTS];
    uint16_t buf[(2 * EV_TARGET_ARM + 1) * 3];   /* large enough for a brush or a crosshair bar */
} evtest_ctx_t;

static evtest_ctx_t *s_ev;

/* The DPI panel copies the bitmap synchronously (use_dma2d is off), so one
 * scratch buffer can be refilled between calls. */
static void ev_fill_rect(int x0, int y0, int x1, int y1, uint16_t color)
{
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    x1 = x1 > s_w ? s_w : x1;
    y1 = y1 > s_h ? s_h : y1;
    int n = (x1 - x0) * (y1 - y0);
    if (n <= 0 || n > (int)(sizeof(s_ev->buf) / sizeof(s_ev->buf[0]))) {
        return;
    }
    for (int i = 0; i < n; i++) {
        s_ev->buf[i] = color;
    }
    esp_lcd_panel_draw_bitmap(s_panel, x0, y0, x1, y1, s_ev->buf);
}

static void ev_draw_target(int i, uint16_t color)
{
    int x = s_ev->tx[i];
    int y = s_ev->ty[i];
    ev_fill_rect(x - EV_TARGET_ARM, y - 1, x + EV_TARGET_ARM + 1, y + 2, color);
    ev_fill_rect(x - 1, y - EV_TARGET_ARM, x + 2, y + EV_TARGET_ARM + 1, color);
}

static void ev_dot(int x, int y, uint16_t color)
{
    ev_fill_rect(x - EV_BRUSH / 2, y - EV_BRUSH / 2, x + EV_BRUSH / 2, y + EV_BRUSH / 2, color);
}

/* Stamp brush dots along the segment so fast strokes stay continuous */
static void ev_stroke(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = (dx > dy ? dx : dy) / (EV_BRUSH / 2);
    for (int k = 1; k <= steps; k++) {
        ev_dot(x0 + (x1 - x0) * k / steps, y0 + (y1 - y0) * k / steps, color);
    }
    ev_dot(x1, y1, color);
}

static void ev_check_targets(int x, int y)
{
    for (int i = 0; i < EV_TARGETS; i++) {
        if (s_ev->hit[i] || abs(x - s_ev->tx[i]) > EV_TARGET_HIT || abs(y - s_ev->ty[i]) > EV_TARGET_HIT) {
            continue;
        }
        s_ev->hit[i] = true;
        s_ev->hits++;
        ev_draw_target(i, RGB565_GREEN);
        printf("evtest: target %d/%d hit at x=%d y=%d\n", s_ev->hits, EV_TARGETS, x, y);
        if (s_ev->hits == EV_TARGETS) {
            printf("evtest: all targets hit, lcd.touch PASS (keep drawing or kill <id>)\n");
            report_set("lcd.touch", REPORT_PASS, "5/5 targets", NULL);
        }
    }
}

static bool evtest_iter(void *ctx)
{
    uint16_t x[EV_MAX_POINTS];
    uint16_t y[EV_MAX_POINTS];
    uint16_t strength[EV_MAX_POINTS];
    uint8_t n = 0;
    esp_lcd_touch_read_data(s_touch);
    if (!esp_lcd_touch_get_coordinates(s_touch, x, y, strength, &n, EV_MAX_POINTS)) {
        n = 0;
    }
    for (int i = 0; i < EV_MAX_POINTS; i++) {
        if (i >= n) {
            if (s_ev->down[i]) {
                printf("touch[%d] up\n", i);
                s_ev->down[i] = false;
            }
            continue;
        }
        int px = x[i] < s_w ? x[i] : s_w - 1;
        int py = y[i] < s_h ? y[i] : s_h - 1;
        if (s_ev->down[i]) {
            ev_stroke(s_ev->last_x[i], s_ev->last_y[i], px, py, s_brush_colors[i]);
        } else {
            printf("touch[%d] down x=%d y=%d\n", i, px, py);
            ev_dot(px, py, s_brush_colors[i]);
            s_ev->down[i] = true;
        }
        s_ev->last_x[i] = px;
        s_ev->last_y[i] = py;
        ev_check_targets(px, py);
    }
    vTaskDelay(pdMS_TO_TICKS(15));
    return true;
}

static void evtest_cleanup(void *ctx)
{
    free(s_ev);
    s_ev = NULL;
}

static void ev_clear_screen(void)
{
    size_t strip_bytes = (size_t)s_w * STRIP_LINES * 2;
    uint16_t *strip = heap_caps_calloc(1, strip_bytes, MALLOC_CAP_SPIRAM);
    if (!strip) {
        return;
    }
    for (int y0 = 0; y0 < s_h; y0 += STRIP_LINES) {
        int lines = (y0 + STRIP_LINES <= s_h) ? STRIP_LINES : s_h - y0;
        esp_lcd_panel_draw_bitmap(s_panel, 0, y0, s_w, y0 + lines, strip);
    }
    free(strip);
}

static int cmd_evtest(int argc, char **argv)
{
    if (display_ensure(NULL, NULL, NULL) != ESP_OK) {
        return 1;
    }
    if (!s_touch) {
        if (esp_board_device_init("lcd_touch") != ESP_OK) {
            printf("touch init failed (GT911 not found on I2C?)\n");
            report_set("lcd.touch", REPORT_FAIL, NULL, "init failed");
            return 1;
        }
        dev_lcd_touch_handles_t *h = NULL;
        esp_board_device_get_handle("lcd_touch", (void **)&h);
        s_touch = h->touch_handle;
    }
    if (s_ev) {
        printf("evtest already running\n");
        return 1;
    }
    s_ev = calloc(1, sizeof(*s_ev));
    if (!s_ev) {
        return 1;
    }
    const int xs[EV_TARGETS] = { EV_TARGET_INSET, s_w - EV_TARGET_INSET, EV_TARGET_INSET, s_w - EV_TARGET_INSET, s_w / 2 };
    const int ys[EV_TARGETS] = { EV_TARGET_INSET, EV_TARGET_INSET, s_h - EV_TARGET_INSET, s_h - EV_TARGET_INSET, s_h / 2 };
    memcpy(s_ev->tx, xs, sizeof(xs));
    memcpy(s_ev->ty, ys, sizeof(ys));

    job_desc_t d = {
        .name = "evtest",
        .iter = evtest_iter,
        .cleanup = evtest_cleanup,
        .ctx = NULL,
        .stack_size = 4096,
        .res = RES_LCD,
    };
    /* Draw the board before the job starts so the first touch lands on it */
    if (!res_lock(RES_LCD, "evtest")) {
        evtest_cleanup(NULL);
        return 1;
    }
    ev_clear_screen();
    for (int i = 0; i < EV_TARGETS; i++) {
        ev_draw_target(i, RGB565_RED);
    }
    res_unlock(RES_LCD);
    report_set("lcd.touch", REPORT_MANUAL, NULL, "touch the 5 targets");
    if (job_start(&d) < 0) {
        evtest_cleanup(NULL);
        return 1;
    }
    printf("draw on the panel and touch the 5 red targets; stop with: kill <id>\n");
    return 0;
}

void register_display_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "fbtest",    .help = "Fill the panel. Usage: fbtest colorbar|red|green|blue|white|black|checker", .func = cmd_fbtest },
        { .command = "backlight", .help = "Set backlight. Usage: backlight <0-100|on|off>", .func = cmd_backlight },
        { .command = "evtest",    .help = "Touch drawing board with 5 targets, until killed", .func = cmd_evtest },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
