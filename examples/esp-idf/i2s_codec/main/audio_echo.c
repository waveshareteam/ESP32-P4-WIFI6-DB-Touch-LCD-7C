/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 */
#include <stdint.h>
#include <stdlib.h>
#include "audio_echo.h"
#include "example_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

#define ECHO_BUFFER_SIZE 2400

static const char *TAG = "audio_echo";
static dev_audio_codec_handles_t *s_adc;
static dev_audio_codec_handles_t *s_dac;
static bool s_adc_open;
static bool s_dac_open;

static void codecs_close(void)
{
    /* Both logical devices share the board I2S/I2C resources. */
    if (s_adc_open) {
        esp_codec_dev_close(s_adc->codec_dev);
        s_adc_open = false;
    }
    if (s_dac_open) {
        esp_codec_dev_close(s_dac->codec_dev);
        s_dac_open = false;
    }
    s_adc = NULL;
    s_dac = NULL;
}

static esp_err_t codec_open(const char *name, dev_audio_codec_handles_t **handles)
{
    void *handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(name, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    *handles = handle;
    if (*handles == NULL || (*handles)->codec_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_codec_dev_sample_info_t format = {
        .sample_rate = EXAMPLE_SAMPLE_RATE,
        .channel = 2,
        .bits_per_sample = 16,
    };
    int result = esp_codec_dev_open((*handles)->codec_dev, &format);
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open %s: %d", name, result);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void echo_task(void *arg)
{
    void *buffer = arg;
    while (1) {
        int ret = esp_codec_dev_read(s_adc->codec_dev, buffer, ECHO_BUFFER_SIZE);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Microphone read failed: %d", ret);
            break;
        }
        ret = esp_codec_dev_write(s_dac->codec_dev, buffer, ECHO_BUFFER_SIZE);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Speaker write failed: %d", ret);
            break;
        }
    }
    free(buffer);
    codecs_close();
    vTaskDelete(NULL);
}

esp_err_t audio_echo_start(void)
{
    if (s_adc != NULL || s_dac != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = codec_open(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, &s_dac);
    if (ret != ESP_OK) {
        goto fail;
    }
    s_dac_open = true;
    ret = codec_open(ESP_BOARD_DEVICE_NAME_AUDIO_ADC, &s_adc);
    if (ret != ESP_OK) {
        goto fail;
    }
    s_adc_open = true;

    int result = esp_codec_dev_set_in_gain(s_adc->codec_dev, EXAMPLE_MIC_GAIN_DB);
    if (result == ESP_CODEC_DEV_OK) {
        /* Set volume after both devices are open, as in the original loopback. */
        result = esp_codec_dev_set_out_vol(s_dac->codec_dev, EXAMPLE_VOICE_VOLUME);
    }
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to configure audio gain/volume: %d", result);
        ret = ESP_FAIL;
        goto fail;
    }

    void *buffer = malloc(ECHO_BUFFER_SIZE);
    if (buffer == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }
    if (xTaskCreate(echo_task, "audio_echo", 8192, buffer, 5, NULL) != pdPASS) {
        free(buffer);
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }
    ESP_LOGI(TAG, "Codec loopback started: 16 kHz, stereo, PCM16, gain=%.0f dB, volume=%d",
             EXAMPLE_MIC_GAIN_DB, EXAMPLE_VOICE_VOLUME);
    return ESP_OK;

fail:
    codecs_close();
    return ret;
}
