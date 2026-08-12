/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "example_config.h"

#include "bsp_board_extra.h"

static const char *TAG = "i2s_es8389";
static const char err_reason[][30] = {"input param is invalid", "operation timeout"};

/* Import music file as buffer */
#if CONFIG_EXAMPLE_MODE_MUSIC
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");
#endif

static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BSP_POWER_AMP_IO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BSP_POWER_AMP_IO, 1);
}

#if CONFIG_EXAMPLE_MODE_MUSIC
static void i2s_music(void *args)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;
    const size_t music_size = music_pcm_end - music_pcm_start;

    while (1) {
        data_ptr = (uint8_t *)music_pcm_start;
        ret = bsp_extra_i2s_write(data_ptr, music_size, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0) {
            ESP_LOGI(TAG, "[music] i2s music played, %u bytes are written.", (unsigned int)bytes_write);
        } else {
            ESP_LOGE(TAG, "[music] i2s music play failed.");
            abort();
        }
        data_ptr = (uint8_t *)music_pcm_start;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

#else
static void update_channel_peaks(const int16_t *mic_data, size_t bytes_read,
                                 uint32_t *left_peak, uint32_t *right_peak)
{
    const size_t sample_count = bytes_read / sizeof(*mic_data);

    for (size_t sample_index = 0; sample_index + 1 < sample_count; sample_index += 2) {
        int32_t left_sample = mic_data[sample_index];
        int32_t right_sample = mic_data[sample_index + 1];

        if (left_sample < 0) {
            left_sample = -left_sample;
        }
        if (right_sample < 0) {
            right_sample = -right_sample;
        }

        if ((uint32_t)left_sample > *left_peak) {
            *left_peak = (uint32_t)left_sample;
        }
        if ((uint32_t)right_sample > *right_peak) {
            *right_peak = (uint32_t)right_sample;
        }
    }
}

static void i2s_echo(void *args)
{
    int16_t *mic_data = malloc(EXAMPLE_RECV_BUF_SIZE);
    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    uint32_t left_peak = 0;
    uint32_t right_peak = 0;
    TickType_t last_level_log = xTaskGetTickCount();
    ESP_LOGI(TAG, "[echo] Echo start");

    while (1) {
        memset(mic_data, 0, EXAMPLE_RECV_BUF_SIZE);
        ret = bsp_extra_i2s_read(mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_read, 1000);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s read failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        update_channel_peaks(mic_data, bytes_read, &left_peak, &right_peak);

        TickType_t current_tick = xTaskGetTickCount();
        if ((current_tick - last_level_log) >= pdMS_TO_TICKS(1000)) {
            ESP_LOGI(TAG, "[echo] captured peak: left=%u, right=%u (PCM16)",
                     (unsigned int)left_peak, (unsigned int)right_peak);
            left_peak = 0;
            right_peak = 0;
            last_level_log = current_tick;
        }

        ret = bsp_extra_i2s_write(mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_write, 1000);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write) {
            ESP_LOGW(TAG, "[echo] %u bytes read but only %u bytes are written", (unsigned int)bytes_read, (unsigned int)bytes_write);
        }
    }
    vTaskDelete(NULL);
}
#endif

void app_main(void)
{
    gpio_init();
    printf("i2s codec example start\n-----------------------------\n");

    if (bsp_extra_codec_init() != ESP_OK) {
        ESP_LOGE(TAG, "codec init failed");
        abort();
    } else {
        ESP_LOGI(TAG, "codec init success");
    }
#if CONFIG_EXAMPLE_MODE_MUSIC
    xTaskCreate(i2s_music, "i2s_music", 4096, NULL, 5, NULL);
#else
    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);
#endif
}
