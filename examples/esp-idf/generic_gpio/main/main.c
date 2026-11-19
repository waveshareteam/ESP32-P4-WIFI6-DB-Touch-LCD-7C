/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"

#include "esp_board_manager_includes.h"
#include "gpio_monitor.h"

#define GPIO_MONITOR_NAME_LEN 8
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static const char *TAG = "generic_gpio";

static const gpio_num_t s_header_gpios[] = {
    GPIO_NUM_32,
    GPIO_NUM_33,
    GPIO_NUM_49,

    GPIO_NUM_50,
    GPIO_NUM_51,
    GPIO_NUM_48,
    GPIO_NUM_47,
    GPIO_NUM_46,
    GPIO_NUM_36,
    GPIO_NUM_25, 
    GPIO_NUM_24, 
    GPIO_NUM_5, 
    GPIO_NUM_4,
};

_Static_assert(GPIO_MODE_INPUT == 1 && GPIO_MODE_OUTPUT == 2, "Kconfig EXAMPLE_GPIO_INITIAL_MODE must match gpio_mode_t");

static gpio_monitor_pin_config_t s_gpio_pins[ARRAY_SIZE(s_header_gpios)];
static char s_gpio_pin_names[ARRAY_SIZE(s_header_gpios)][GPIO_MONITOR_NAME_LEN];

static dev_display_lcd_handles_t *s_lcd;
static dev_display_lcd_config_t *s_lcd_config;

static esp_lcd_touch_handle_t s_touch;
static bool s_lcd_initialized;
static bool s_brightness_initialized;
static bool s_touch_initialized;

static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;
static bool s_lv_adapter_initialized;

static esp_err_t gpio_monitor_init_from_header(void)
{
    const size_t gpio_count = ARRAY_SIZE(s_header_gpios);

    for (size_t i = 0; i < gpio_count; i++) {
        snprintf(s_gpio_pin_names[i], sizeof(s_gpio_pin_names[i]), "GPIO%d", s_header_gpios[i]);
        s_gpio_pins[i] = (gpio_monitor_pin_config_t){
            .name = s_gpio_pin_names[i],
            .gpio_num = s_header_gpios[i],
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLUP_DISABLE,
            .initial_level = 0,
        };
    }

    const gpio_monitor_config_t config = {
        .pins = s_gpio_pins,
        .pin_count = gpio_count,
        .initial_mode = (gpio_mode_t)CONFIG_EXAMPLE_GPIO_INITIAL_MODE,
        .refresh_period_ms = 100,
    };
    return gpio_monitor_init(&config);
}

static esp_err_t board_display_start(void)
{
    /* The board definition configures three DPI frame buffers, as the triple-buffer tear avoidance mode requires. */
    esp_err_t ret = esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&s_lcd_config);
    if (ret != ESP_OK || s_lcd_config == NULL) {
        ESP_LOGE(TAG, "Failed to get LCD config: %s", esp_err_to_name(ret));
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD: %s", esp_err_to_name(ret));
        return ret;
    }
    s_lcd_initialized = true;

    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&s_lcd);
    if (ret != ESP_OK || s_lcd == NULL || s_lcd->panel_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get LCD handles: %s", esp_err_to_name(ret));
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    if (ret == ESP_OK) {
        s_touch_initialized = true;
        dev_lcd_touch_handles_t *touch_handles = NULL;
        ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_LCD_TOUCH, (void **)&touch_handles);
        if (ret == ESP_OK && touch_handles != NULL) {
            s_touch = touch_handles->touch_handle;
        }
    }
    if (s_touch == NULL) {
        ESP_LOGW(TAG, "Failed to initialize touch; continue without touch: %s", esp_err_to_name(ret));
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on backlight: %s", esp_err_to_name(ret));
        return ret;
    }
    s_brightness_initialized = true;

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL adapter: %s", esp_err_to_name(ret));
        return ret;
    }
    s_lv_adapter_initialized = true;

    esp_lv_adapter_display_config_t display_config = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(s_lcd->panel_handle, s_lcd->io_handle, s_lcd_config->lcd_width, s_lcd_config->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
    display_config.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    s_display = esp_lv_adapter_register_display(&display_config);
    if (s_display == NULL) {
        ESP_LOGE(TAG, "Failed to register LCD with LVGL adapter");
        return ESP_FAIL;
    }

    if (s_touch != NULL) {
        const esp_lv_adapter_touch_config_t touch_config = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_display, s_touch);
        s_touch_indev = esp_lv_adapter_register_touch(&touch_config);
        if (s_touch_indev == NULL) {
            ESP_LOGW(TAG, "Failed to register touch; continue without touch");
        }
    }

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL adapter: %s", esp_err_to_name(ret));
    }
    return ret;
}

static void board_display_stop(void)
{
    if (s_touch_indev != NULL) {
        esp_lv_adapter_unregister_touch(s_touch_indev);
        s_touch_indev = NULL;
    }
    if (s_display != NULL) {
        esp_lv_adapter_unregister_display(s_display);
        s_display = NULL;
    }
    if (s_lv_adapter_initialized) {
        esp_lv_adapter_deinit();
        s_lv_adapter_initialized = false;
    }

    if (s_brightness_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
        s_brightness_initialized = false;
    }
    if (s_touch_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
        s_touch_initialized = false;
    }
    s_touch = NULL;
    if (s_lcd_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
        s_lcd_initialized = false;
    }
    s_lcd = NULL;
}

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_monitor_init_from_header());

    esp_err_t ret = board_display_start();
    if (ret != ESP_OK) {
        board_display_stop();
        ESP_ERROR_CHECK(ret);
    }

    esp_lv_adapter_lock(-1);
    ret = gpio_monitor_start_ui();
    esp_lv_adapter_unlock();
    if (ret != ESP_OK) {
        board_display_stop();
        ESP_ERROR_CHECK(ret);
    }
}
