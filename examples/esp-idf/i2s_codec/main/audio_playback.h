/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: CC0-1.0
 */
#pragma once
#include "esp_err.h"

/* Board DAC must already be initialized. */
esp_err_t audio_playback_start(void);
