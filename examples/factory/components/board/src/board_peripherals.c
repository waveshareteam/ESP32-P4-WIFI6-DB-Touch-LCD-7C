/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_peripherals.h"

#include "esp_check.h"
#include "esp_twai_onchip.h"


static const char *TAG = "board_periph";

esp_err_t board_twai_new_node(uint32_t bitrate, uint32_t tx_queue_depth, bool listen_only,
                              twai_node_handle_t *ret_node)
{
    ESP_RETURN_ON_FALSE(ret_node, ESP_ERR_INVALID_ARG, TAG, "Node handle is NULL");

    twai_onchip_node_config_t config = {};
    config.io_cfg.tx = listen_only ? GPIO_NUM_NC : BOARD_TWAI_TX_GPIO;
    config.io_cfg.rx = BOARD_TWAI_RX_GPIO;
    config.io_cfg.quanta_clk_out = GPIO_NUM_NC;
    config.io_cfg.bus_off_indicator = GPIO_NUM_NC;
    config.bit_timing.bitrate = bitrate;
    config.timestamp_resolution_hz = 1000000;
    // A bounded retry count keeps one missing ACK from holding a TX slot forever.
    config.fail_retry_cnt = 3;
    config.tx_queue_depth = tx_queue_depth;
    config.flags.enable_listen_only = listen_only;
    return twai_new_node_onchip(&config, ret_node);
}
