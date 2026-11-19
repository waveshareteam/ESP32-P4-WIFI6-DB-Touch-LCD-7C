/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;   /*!< 2000 to 2099 */
    uint8_t month;   /*!< 1 to 12 */
    uint8_t day;     /*!< 1 to 31 */
    uint8_t weekday; /*!< 0 = Sunday */
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} board_rtc_time_t;

/** Read the on-board PCF85063A. Initializes the RTC on first use. */
esp_err_t board_rtc_get_time(board_rtc_time_t *time);

/** Write the on-board PCF85063A. The fields are validated before writing. */
esp_err_t board_rtc_set_time(const board_rtc_time_t *time);

#ifdef __cplusplus
}
#endif
