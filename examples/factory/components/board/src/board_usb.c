/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_usb.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_private/usb_phy.h"

static const char *TAG = "board_usb";

static usb_phy_handle_t s_usb_phy = NULL;

esp_err_t board_usb_get_pins(board_usb_pins_t *pins)
{
    ESP_RETURN_ON_FALSE(pins, ESP_ERR_INVALID_ARG, TAG, "USB pins pointer is NULL");
    *pins = (board_usb_pins_t) {
        .fs_dm_gpio = BOARD_USB_FS_DM_GPIO,
        .fs_dp_gpio = BOARD_USB_FS_DP_GPIO,
        .hs_utmi_supported = true,
        .fs_internal_supported = true,
    };
    return ESP_OK;
}

esp_err_t board_usb_phy_init(board_usb_port_t port, board_usb_mode_t mode)
{
    if (s_usb_phy != NULL) {
        return ESP_OK;
    }

    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = (mode == BOARD_USB_MODE_HOST) ? USB_OTG_MODE_HOST : USB_OTG_MODE_DEVICE,
        .target = (port == BOARD_USB_PORT_HS_2_0) ? USB_PHY_TARGET_UTMI : USB_PHY_TARGET_INT,
        .otg_speed = (port == BOARD_USB_PORT_HS_2_0) ? USB_PHY_SPEED_HIGH : USB_PHY_SPEED_FULL,
    };

    ESP_RETURN_ON_ERROR(usb_new_phy(&phy_conf, &s_usb_phy), TAG, "Initialize USB PHY failed");
    ESP_LOGI(TAG, "USB PHY initialized: %s (%s)",
             port == BOARD_USB_PORT_HS_2_0 ? "HS 2.0 UTMI" : "FS 1.1 Internal",
             mode == BOARD_USB_MODE_HOST ? "Host" : "Device");
    return ESP_OK;
}

esp_err_t board_usb_phy_deinit(void)
{
    if (s_usb_phy == NULL) {
        return ESP_OK;
    }

    esp_err_t ret = usb_del_phy(s_usb_phy);
    if (ret == ESP_OK) {
        s_usb_phy = NULL;
    }
    return ret;
}

bool board_usb_phy_is_initialized(void)
{
    return s_usb_phy != NULL;
}
