/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of user LEDs (user_led_1 and user_led_2 in board_devices.yaml). */
#define BOARD_USER_LED_COUNT (2)

/** Turn a user LED on or off. index is 0-based. The active level comes from the board definition. */
esp_err_t board_user_led_set(int index, bool on);

/** Read back the last state written with board_user_led_set(). */
esp_err_t board_user_led_get(int index, bool *on);

#ifdef __cplusplus
}
#endif
