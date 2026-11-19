#include <stdbool.h>

#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "lv_demos.h"

static const char *TAG = "lv_benchmark";

static bool s_lcd_initialized;
static bool s_touch_initialized;
static bool s_brightness_initialized;
static bool s_adapter_initialized;
static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;

static esp_err_t board_display_start(void)
{
    dev_display_lcd_config_t *lcd_config = NULL;
    dev_display_lcd_handles_t *lcd_handles = NULL;
    esp_err_t ret = esp_board_manager_get_device_config(
        ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_config);
    if (ret != ESP_OK || lcd_config == NULL) {
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        return ret;
    }
    s_lcd_initialized = true;

    ret = esp_board_manager_get_device_handle(
        ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_handles);
    if (ret != ESP_OK || lcd_handles == NULL || lcd_handles->panel_handle == NULL) {
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    if (ret != ESP_OK) {
        return ret;
    }
    s_brightness_initialized = true;

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
    if (ret == ESP_OK) {
        s_touch_initialized = true;
    } else {
        ESP_LOGW(TAG, "Touch initialization failed; continuing without touch: %s",
                 esp_err_to_name(ret));
    }

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) {
        return ret;
    }
    s_adapter_initialized = true;

    esp_lv_adapter_display_config_t display_config = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        lcd_handles->panel_handle, lcd_handles->io_handle,
        lcd_config->lcd_width, lcd_config->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
    display_config.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    s_display = esp_lv_adapter_register_display(&display_config);
    if (s_display == NULL) {
        return ESP_FAIL;
    }

    if (s_touch_initialized) {
        dev_lcd_touch_handles_t *touch_handles = NULL;
        ret = esp_board_manager_get_device_handle(
            ESP_BOARD_DEVICE_NAME_LCD_TOUCH, (void **)&touch_handles);
        if (ret == ESP_OK && touch_handles != NULL && touch_handles->touch_handle != NULL) {
            const esp_lv_adapter_touch_config_t touch_config =
                ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(s_display, touch_handles->touch_handle);
            s_touch_indev = esp_lv_adapter_register_touch(&touch_config);
            if (s_touch_indev == NULL) {
                ESP_LOGW(TAG, "Touch registration failed; continuing without touch");
            }
        }
    }

    return esp_lv_adapter_start();
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
    if (s_adapter_initialized) {
        esp_lv_adapter_deinit();
        s_adapter_initialized = false;
    }
    if (s_touch_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_TOUCH);
        s_touch_initialized = false;
    }
    if (s_brightness_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
        s_brightness_initialized = false;
    }
    if (s_lcd_initialized) {
        esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
        s_lcd_initialized = false;
    }
}

void app_main(void)
{
    esp_err_t ret = board_display_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize board display: %s", esp_err_to_name(ret));
        board_display_stop();
        return;
    }

    ret = esp_lv_adapter_lock(-1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock LVGL: %s", esp_err_to_name(ret));
        board_display_stop();
        return;
    }
    lv_demo_benchmark();
    esp_lv_adapter_unlock();
    ESP_LOGI(TAG, "LVGL benchmark started");
}
