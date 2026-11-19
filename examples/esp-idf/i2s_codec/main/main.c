/*
 * SPDX-FileCopyrightText: 2021-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 */
#include <stddef.h>
#include "sdkconfig.h"
#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_log.h"

#if CONFIG_EXAMPLE_MODE_ECHO
#include "audio_echo.h"
#else
#include "audio_playback.h"
#endif

void app_main(void)
{
    static const char *TAG = "i2s_codec";
    const char *devices[] = {
        ESP_BOARD_DEVICE_NAME_AUDIO_DAC,
#if CONFIG_EXAMPLE_MODE_ECHO
        ESP_BOARD_DEVICE_NAME_AUDIO_ADC,
#endif
    };
    size_t initialized = 0;
    esp_err_t ret = ESP_OK;
    for (; initialized < sizeof(devices) / sizeof(devices[0]); ++initialized) {
        ret = esp_board_manager_init_device_by_name(devices[initialized]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize %s: %s", devices[initialized], esp_err_to_name(ret));
            break;
        }
    }
    if (ret == ESP_OK) {
#if CONFIG_EXAMPLE_MODE_ECHO
        ret = audio_echo_start();
#else
        ret = audio_playback_start();
#endif
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Audio example failed: %s", esp_err_to_name(ret));
        while (initialized > 0) {
            esp_board_manager_deinit_device_by_name(devices[--initialized]);
        }
    }
}
