#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "cmd_led.h"
#include "jobs.h"
#include "report.h"

/* User LEDs as in the BSP (BSP_USER1_LED / BSP_USER2_LED), both active low */
#define LED_COUNT        2
#define LED_ON_LEVEL     0
#define LED_BLINK_MS     500

static const int s_led_pins[LED_COUNT] = { 2, 3 };
static bool s_led_ready;

static void led_init_once(void)
{
    if (s_led_ready) {
        return;
    }
    for (int i = 0; i < LED_COUNT; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << s_led_pins[i],
            .mode = GPIO_MODE_INPUT_OUTPUT,   /* input enabled so the state can be read back */
        };
        gpio_config(&cfg);
        gpio_set_level(s_led_pins[i], !LED_ON_LEVEL);
    }
    s_led_ready = true;
}

static void led_set(int idx, bool on)
{
    gpio_set_level(s_led_pins[idx], on ? LED_ON_LEVEL : !LED_ON_LEVEL);
}

static bool led_is_on(int idx)
{
    return gpio_get_level(s_led_pins[idx]) == LED_ON_LEVEL;
}

/* ---- led blink: LED1 and LED2 alternate at 1 Hz until killed ---- */
static bool blink_iter(void *arg)
{
    int *phase = arg;
    *phase ^= 1;
    led_set(0, *phase);
    led_set(1, !*phase);
    vTaskDelay(pdMS_TO_TICKS(LED_BLINK_MS));
    return true;
}

static void blink_cleanup(void *arg)
{
    for (int i = 0; i < LED_COUNT; i++) {
        led_set(i, false);
    }
    free(arg);
}

/* led | led <1|2|all> <on|off> | led blink */
static int cmd_led(int argc, char **argv)
{
    led_init_once();
    if (argc == 1) {
        for (int i = 0; i < LED_COUNT; i++) {
            printf("LED%d (GPIO%d): %s\n", i + 1, s_led_pins[i], led_is_on(i) ? "on" : "off");
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "blink") == 0) {
        if (job_find("led blink") > 0) {
            printf("led blink already running\n");
            return 1;
        }
        int *phase = calloc(1, sizeof(int));
        if (!phase) {
            return 1;
        }
        job_desc_t d = {
            .name = "led blink",
            .iter = blink_iter,
            .cleanup = blink_cleanup,
            .ctx = phase,
            .stack_size = 3072,
            .res = RES_MAX,
        };
        if (job_start(&d) < 0) {
            free(phase);
            return 1;
        }
        printf("LED1 / LED2 blinking alternately; stop with kill <id>\n");
        report_set("led.user", REPORT_MANUAL, NULL, "check both LEDs blink");
        return 0;
    }
    if (argc == 3 && (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "off") == 0)) {
        bool on = strcmp(argv[2], "on") == 0;
        int first;
        int last;
        if (strcmp(argv[1], "all") == 0) {
            first = 0;
            last = LED_COUNT - 1;
        } else {
            first = last = atoi(argv[1]) - 1;
            if (first < 0 || first >= LED_COUNT) {
                printf("LED must be 1, 2 or all\n");
                return 1;
            }
        }
        if (job_find("led blink") > 0) {
            printf("led blink is running, kill it first\n");
            return 1;
        }
        for (int i = first; i <= last; i++) {
            led_set(i, on);
        }
        report_set("led.user", REPORT_MANUAL, NULL, "check LEDs visually");
        return 0;
    }
    printf("usage: led | led <1|2|all> <on|off> | led blink\n");
    return 1;
}

void register_led_commands(void)
{
    const esp_console_cmd_t cmd = {
        .command = "led",
        .help = "User LEDs (LED1 GPIO2, LED2 GPIO3). led | led <1|2|all> <on|off> | led blink",
        .func = cmd_led,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
