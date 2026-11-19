/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_board_manager_includes.h"

static const char *TAG = "mipi_dsi_test";

void app_main(void)
{
    dev_display_lcd_handles_t *lcd = NULL;

    ESP_LOGI(TAG, "Initialize LCD");
    esp_err_t ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd);
    if (ret != ESP_OK || lcd == NULL || lcd->panel_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get LCD handles: %s", esp_err_to_name(ret));
        goto cleanup_lcd;
    }

    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on backlight: %s", esp_err_to_name(ret));
        goto cleanup_lcd;
    }

    ESP_LOGI(TAG, "Show color bar pattern drawn by hardware");
    ret = esp_lcd_dpi_panel_set_pattern(lcd->panel_handle, MIPI_DSI_PATTERN_BAR_VERTICAL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LCD color bar pattern: %s", esp_err_to_name(ret));
        goto cleanup_backlight;
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    return;

cleanup_backlight:
    esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS);
cleanup_lcd:
    esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD);
}
