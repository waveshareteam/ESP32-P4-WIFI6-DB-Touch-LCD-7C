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

/**
 * @brief Initialize the on-board BOOT button (GPIO35, active-low).
 *
 * Uses the esp_board_manager boot_button device when available (and registers
 * default single-click / long-press log callbacks), falling back to direct
 * GPIO input polling otherwise.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Initialization failed
 */
esp_err_t board_button_init(void);

/**
 * @brief Deinitialize the BOOT button resources.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Deinitialization failed
 */
esp_err_t board_button_deinit(void);

/**
 * @brief Read whether the BOOT button is currently pressed.
 *
 * @param[out] pressed true if the BOOT button is pressed, false otherwise
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_ARG: pressed is NULL
 *    - ESP_ERR_INVALID_STATE: Button is not initialized
 */
esp_err_t board_button_is_boot_pressed(bool *pressed);

typedef void (*board_button_click_cb_t)(void *arg);

/**
 * @brief Register or unregister a single-click callback for the BOOT button.
 *
 * Passing NULL as cb unregisters any previously registered callback.
 */
esp_err_t board_button_register_boot_click_cb(board_button_click_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif
