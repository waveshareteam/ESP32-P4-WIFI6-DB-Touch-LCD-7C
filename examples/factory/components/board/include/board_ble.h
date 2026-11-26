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
 * @brief Initialize the ESP-Hosted BLE controller binding and NimBLE host stack.
 *
 * Coordinates with the shared ESP-Hosted SDIO transport used by Wi-Fi.
 * Repeated calls return ESP_OK while BLE remains initialized.
 */
esp_err_t board_ble_init(void);

/**
 * @brief Stop and deinitialize the NimBLE host stack.
 */
esp_err_t board_ble_deinit(void);

/**
 * @brief Check whether the board BLE stack is initialized.
 */
bool board_ble_is_initialized(void);

#ifdef __cplusplus
}
#endif
