/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_twai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ESP32-P4-WIFI6-DB-Touch-LCD-7C: on-board CAN transceiver. */
#define BOARD_TWAI_TX_GPIO (GPIO_NUM_22)
#define BOARD_TWAI_RX_GPIO (GPIO_NUM_21)

/**
 * Create the on-board TWAI node (esp_twai on-chip driver). The node is returned
 * disabled; the caller registers its callbacks and enables it.
 * A listen-only node leaves the TX pin unrouted so it cannot drive the bus.
 */
esp_err_t board_twai_new_node(uint32_t bitrate, uint32_t tx_queue_depth, bool listen_only,
                              twai_node_handle_t *ret_node);

#ifdef __cplusplus
}
#endif
