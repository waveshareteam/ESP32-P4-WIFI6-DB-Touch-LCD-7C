/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The board's ES8389 path uses a stereo PCM stream. */
#define XIAOZHI_AUDIO_DEVICE_SAMPLE_RATE     (16000U)
#define XIAOZHI_AUDIO_DEVICE_CHANNELS        (2U)
#define XIAOZHI_AUDIO_DEVICE_BITS_PER_SAMPLE (16U)

/** Open the requested native BSP audio devices for Xiaozhi. */
esp_err_t xiaozhi_audio_start(bool enable_input, bool enable_output);

/** Close the native BSP audio devices used by Xiaozhi. */
esp_err_t xiaozhi_audio_stop(void);

/** Read interleaved stereo PCM from the board microphone. */
esp_err_t xiaozhi_audio_read(
    void *audio_buffer,
    size_t len,
    size_t *bytes_read,
    uint32_t timeout_ms
);

/** Write interleaved stereo PCM to the board speaker. */
esp_err_t xiaozhi_audio_write(
    const void *audio_buffer,
    size_t len,
    size_t *bytes_written,
    uint32_t timeout_ms
);

#ifdef __cplusplus
}
#endif
