/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"

/* Umbrella header for the board support layer. */
#include "board_display.h"
#include "board_leds.h"
#include "board_rtc.h"
#include "board_camera.h"
#include "board_audio.h"
#include "board_storage.h"
#include "board_wifi.h"
#include "board_ble.h"
#include "board_usb.h"
#include "board_button.h"
#include "board_peripherals.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Board identity metadata from esp_board_manager. */
typedef struct {
    const char *name;
    const char *chip;
    const char *version;
    const char *description;
    const char *manufacturer;
} board_info_t;

/**
 * @brief Get the board identity metadata from esp_board_manager.
 *
 * @param[out] info Pointer to board_info_t to populate
 * @return ESP_OK on success, or an error from esp_board_manager
 */
esp_err_t board_get_info(board_info_t *info);

#ifdef __cplusplus
}
#endif
