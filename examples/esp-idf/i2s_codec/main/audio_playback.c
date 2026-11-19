/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 */
#include <stdint.h>
#include "audio_playback.h"
#include "example_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

#define PLAYBACK_CHUNK_SIZE 4096

static const char *TAG = "audio_playback";
static dev_audio_codec_handles_t *s_dac;

extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[] asm("_binary_canon_pcm_end");

static void playback_task(void *arg)
{
    (void)arg;
    const size_t size = music_pcm_end - music_pcm_start;
    size_t offset = 0;
    while (1) {
        size_t bytes = size - offset;
        if (bytes > PLAYBACK_CHUNK_SIZE) {
            bytes = PLAYBACK_CHUNK_SIZE;
        }
        /* The write blocks on the I2S DMA, so it paces the loop. */
        int ret = esp_codec_dev_write(s_dac->codec_dev, (void *)(music_pcm_start + offset), bytes);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Speaker write failed: %d", ret);
            break;
        }
        offset += bytes;
        if (offset >= size) {
            offset = 0;
        }
    }
    esp_codec_dev_close(s_dac->codec_dev);
    s_dac = NULL;
    vTaskDelete(NULL);
}

esp_err_t audio_playback_start(void)
{
    if (s_dac != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    void *handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    dev_audio_codec_handles_t *dac = handle;
    if (dac == NULL || dac->codec_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_codec_dev_sample_info_t format = {
        .sample_rate = EXAMPLE_SAMPLE_RATE,
        .channel = 2,
        .bits_per_sample = 16,
    };
    int result = esp_codec_dev_open(dac->codec_dev, &format);
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open DAC: %d", result);
        return ESP_FAIL;
    }
    result = esp_codec_dev_set_out_vol(dac->codec_dev, EXAMPLE_VOICE_VOLUME);
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to set volume: %d", result);
        esp_codec_dev_close(dac->codec_dev);
        return ESP_FAIL;
    }
    s_dac = dac;
    if (xTaskCreate(playback_task, "audio_playback", 4096, NULL, 5, NULL) != pdPASS) {
        esp_codec_dev_close(dac->codec_dev);
        s_dac = NULL;
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "PCM playback started: 16 kHz, stereo, PCM16, volume=%d", EXAMPLE_VOICE_VOLUME);
    return ESP_OK;
}
