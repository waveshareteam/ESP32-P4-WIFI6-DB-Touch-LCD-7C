/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* USB 1.1 Full-Speed D-/D+ routed to P1 header (pins 3 and 4). */
#define BOARD_USB_FS_DM_GPIO (GPIO_NUM_24)
#define BOARD_USB_FS_DP_GPIO (GPIO_NUM_25)

/** @brief On-board USB PHY ports. */
typedef enum {
    BOARD_USB_PORT_HS_2_0 = 0,  /*!< USB 2.0 High-Speed UTMI PHY (dedicated pins) */
    BOARD_USB_PORT_FS_1_1,      /*!< USB 1.1 Full-Speed internal PHY (GPIO24/GPIO25) */
} board_usb_port_t;

/** @brief USB OTG role for PHY initialization. */
typedef enum {
    BOARD_USB_MODE_DEVICE = 0,
    BOARD_USB_MODE_HOST,
} board_usb_mode_t;

/** @brief Board USB pin and PHY capability description. */
typedef struct {
    gpio_num_t fs_dm_gpio;
    gpio_num_t fs_dp_gpio;
    bool       hs_utmi_supported;
    bool       fs_internal_supported;
} board_usb_pins_t;

/**
 * @brief Query the board USB pin assignments and PHY capabilities.
 */
esp_err_t board_usb_get_pins(board_usb_pins_t *pins);

/**
 * @brief Initialize the selected on-chip USB PHY in device or host mode.
 *
 * Repeated calls return ESP_OK while a PHY handle is active.
 */
esp_err_t board_usb_phy_init(board_usb_port_t port, board_usb_mode_t mode);

/**
 * @brief Delete the active USB PHY handle initialized by board_usb_phy_init().
 */
esp_err_t board_usb_phy_deinit(void);

/**
 * @brief Check whether a board USB PHY handle is currently initialized.
 */
bool board_usb_phy_is_initialized(void);

#ifdef __cplusplus
}
#endif
