/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xiaozhi_audio.h"

#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define XIAOZHI_AUDIO_DEFAULT_VOLUME (80)
#define XIAOZHI_AUDIO_MIC_GAIN_DB    (24.0F)

static esp_codec_dev_handle_t s_speaker_device;
static esp_codec_dev_handle_t s_microphone_device;
static SemaphoreHandle_t s_audio_mutex;
static SemaphoreHandle_t s_input_mutex;
static SemaphoreHandle_t s_output_mutex;
static portMUX_TYPE s_mutex_init_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_session_active;
static bool s_session_stopping;
static bool s_input_enabled;
static bool s_output_enabled;
static bool s_speaker_opened;
static bool s_microphone_opened;
static uint32_t s_input_in_flight;
static uint32_t s_output_in_flight;

static esp_err_t ensure_audio_mutexes(void)
{
    taskENTER_CRITICAL(&s_mutex_init_lock);
    bool mutexes_ready = s_audio_mutex != NULL && s_input_mutex != NULL &&
                         s_output_mutex != NULL;
    taskEXIT_CRITICAL(&s_mutex_init_lock);
    if (mutexes_ready) {
        return ESP_OK;
    }

    SemaphoreHandle_t audio_mutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t input_mutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t output_mutex = xSemaphoreCreateMutex();
    if (audio_mutex == NULL || input_mutex == NULL || output_mutex == NULL) {
        if (audio_mutex != NULL) {
            vSemaphoreDelete(audio_mutex);
        }
        if (input_mutex != NULL) {
            vSemaphoreDelete(input_mutex);
        }
        if (output_mutex != NULL) {
            vSemaphoreDelete(output_mutex);
        }
        return ESP_ERR_NO_MEM;
    }

    taskENTER_CRITICAL(&s_mutex_init_lock);
    if (s_audio_mutex == NULL) {
        s_audio_mutex = audio_mutex;
        audio_mutex = NULL;
    }
    if (s_input_mutex == NULL) {
        s_input_mutex = input_mutex;
        input_mutex = NULL;
    }
    if (s_output_mutex == NULL) {
        s_output_mutex = output_mutex;
        output_mutex = NULL;
    }
    taskEXIT_CRITICAL(&s_mutex_init_lock);

    if (audio_mutex != NULL) {
        vSemaphoreDelete(audio_mutex);
    }
    if (input_mutex != NULL) {
        vSemaphoreDelete(input_mutex);
    }
    if (output_mutex != NULL) {
        vSemaphoreDelete(output_mutex);
    }
    return ESP_OK;
}

static esp_err_t lock_audio(TickType_t timeout)
{
    esp_err_t result = ensure_audio_mutexes();
    if (result != ESP_OK) {
        return result;
    }
    return xSemaphoreTake(s_audio_mutex, timeout) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void unlock_audio(void)
{
    xSemaphoreGive(s_audio_mutex);
}

static esp_err_t close_audio_devices_locked(void)
{
    esp_err_t result = ESP_OK;
    if (s_microphone_opened) {
        result = esp_codec_dev_close(s_microphone_device);
        if (result == ESP_OK) {
            s_microphone_opened = false;
        }
    }
    if (s_speaker_opened) {
        esp_err_t speaker_result = esp_codec_dev_close(s_speaker_device);
        if (speaker_result == ESP_OK) {
            s_speaker_opened = false;
        }
        if (result == ESP_OK) {
            result = speaker_result;
        }
    }
    return result;
}

static esp_err_t create_audio_devices_locked(bool enable_input, bool enable_output)
{
    if (enable_output && s_speaker_device == NULL) {
        s_speaker_device = bsp_audio_codec_speaker_init();
        if (s_speaker_device == NULL) {
            return ESP_FAIL;
        }
    }
    if (enable_input && s_microphone_device == NULL) {
        s_microphone_device = bsp_audio_codec_microphone_init();
        if (s_microphone_device == NULL) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t open_audio_devices_locked(bool enable_input, bool enable_output)
{
    const esp_codec_dev_sample_info_t sample_info = {
        .sample_rate = XIAOZHI_AUDIO_DEVICE_SAMPLE_RATE,
        .channel = I2S_SLOT_MODE_STEREO,
        .bits_per_sample = XIAOZHI_AUDIO_DEVICE_BITS_PER_SAMPLE,
    };
    esp_err_t result = close_audio_devices_locked();
    if (result != ESP_OK) {
        return result;
    }
    result = create_audio_devices_locked(enable_input, enable_output);
    if (result != ESP_OK) {
        return result;
    }

    if (enable_output) {
        result = esp_codec_dev_open(s_speaker_device, &sample_info);
        if (result != ESP_OK) {
            return result;
        }
        s_speaker_opened = true;
        result = esp_codec_dev_set_out_mute(s_speaker_device, false);
        if (result == ESP_OK) {
            result = esp_codec_dev_set_out_vol(
                         s_speaker_device, XIAOZHI_AUDIO_DEFAULT_VOLUME
                     );
        }
        if (result != ESP_OK) {
            (void)close_audio_devices_locked();
            return result;
        }
    }

    if (!enable_input) {
        return ESP_OK;
    }
    result = esp_codec_dev_open(s_microphone_device, &sample_info);
    if (result == ESP_OK) {
        s_microphone_opened = true;
        result = bsp_audio_codec_configure_microphone(
                     s_microphone_device, XIAOZHI_AUDIO_MIC_GAIN_DB
                 );
    }
    if (result != ESP_OK) {
        (void)close_audio_devices_locked();
    }
    return result;
}

static esp_err_t begin_audio_io(bool input)
{
    esp_err_t result = lock_audio(portMAX_DELAY);
    if (result != ESP_OK) {
        return result;
    }
    bool enabled = input ? s_input_enabled : s_output_enabled;
    bool opened = input ? s_microphone_opened : s_speaker_opened;
    if (!s_session_active || s_session_stopping || !enabled || !opened) {
        unlock_audio();
        return ESP_ERR_INVALID_STATE;
    }
    if (input) {
        ++s_input_in_flight;
    } else {
        ++s_output_in_flight;
    }
    unlock_audio();
    return ESP_OK;
}

static void end_audio_io(bool input)
{
    if (lock_audio(portMAX_DELAY) != ESP_OK) {
        return;
    }
    uint32_t *in_flight = input ? &s_input_in_flight : &s_output_in_flight;
    if (*in_flight > 0) {
        --(*in_flight);
    }
    unlock_audio();
}

esp_err_t xiaozhi_audio_start(bool enable_input, bool enable_output)
{
    if (!enable_input && !enable_output) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t result = lock_audio(portMAX_DELAY);
    if (result != ESP_OK) {
        return result;
    }
    if (s_session_active || s_session_stopping) {
        unlock_audio();
        return ESP_ERR_INVALID_STATE;
    }

    s_session_stopping = true;
    result = open_audio_devices_locked(enable_input, enable_output);
    if (result == ESP_OK) {
        s_input_enabled = enable_input;
        s_output_enabled = enable_output;
        s_session_active = true;
    }
    s_session_stopping = false;
    unlock_audio();
    return result;
}

esp_err_t xiaozhi_audio_stop(void)
{
    esp_err_t result = lock_audio(portMAX_DELAY);
    if (result != ESP_OK) {
        return result;
    }
    if (!s_session_active) {
        unlock_audio();
        return ESP_OK;
    }
    s_session_stopping = true;
    while (s_input_in_flight != 0 || s_output_in_flight != 0) {
        unlock_audio();
        vTaskDelay(pdMS_TO_TICKS(2));
        result = lock_audio(portMAX_DELAY);
        if (result != ESP_OK) {
            return result;
        }
    }
    result = close_audio_devices_locked();
    s_input_enabled = false;
    s_output_enabled = false;
    s_session_active = false;
    s_session_stopping = false;
    unlock_audio();
    return result;
}

esp_err_t xiaozhi_audio_read(
    void *audio_buffer,
    size_t len,
    size_t *bytes_read,
    uint32_t timeout_ms
)
{
    if (bytes_read != NULL) {
        *bytes_read = 0;
    }
    esp_err_t result = ensure_audio_mutexes();
    if (audio_buffer == NULL || len == 0 || result != ESP_OK) {
        return audio_buffer == NULL || len == 0 ? ESP_ERR_INVALID_ARG : result;
    }
    TickType_t timeout = timeout_ms == UINT32_MAX ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(s_input_mutex, timeout) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    result = begin_audio_io(true);
    if (result == ESP_OK) {
        result = esp_codec_dev_read(s_microphone_device, audio_buffer, len);
        if (result == ESP_OK && bytes_read != NULL) {
            *bytes_read = len;
        }
        end_audio_io(true);
    }
    xSemaphoreGive(s_input_mutex);
    return result;
}

esp_err_t xiaozhi_audio_write(
    const void *audio_buffer,
    size_t len,
    size_t *bytes_written,
    uint32_t timeout_ms
)
{
    if (bytes_written != NULL) {
        *bytes_written = 0;
    }
    esp_err_t result = ensure_audio_mutexes();
    if (audio_buffer == NULL || len == 0 || result != ESP_OK) {
        return audio_buffer == NULL || len == 0 ? ESP_ERR_INVALID_ARG : result;
    }
    TickType_t timeout = timeout_ms == UINT32_MAX ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(s_output_mutex, timeout) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    result = begin_audio_io(false);
    if (result == ESP_OK) {
        result = esp_codec_dev_write(s_speaker_device, (void *)audio_buffer, len);
        if (result == ESP_OK && bytes_written != NULL) {
            *bytes_written = len;
        }
        end_audio_io(false);
    }
    xSemaphoreGive(s_output_mutex);
    return result;
}
