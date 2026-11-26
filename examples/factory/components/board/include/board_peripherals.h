/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_twai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ESP32-P4-WIFI6-DB-Touch-LCD-7C: on-board CAN transceiver. */
#define BOARD_TWAI_TX_GPIO (GPIO_NUM_22)
#define BOARD_TWAI_RX_GPIO (GPIO_NUM_21)

/**
 * @brief Get the TWAI/CAN TX and RX GPIOs from the board configuration.
 *
 * Falls back to BOARD_TWAI_TX_GPIO / BOARD_TWAI_RX_GPIO if no "can" device is configured.
 *
 * @param[out] tx_gpio TWAI TX GPIO number (may be NULL)
 * @param[out] rx_gpio TWAI RX GPIO number (may be NULL)
 * @return ESP_OK on success
 */
esp_err_t board_twai_get_pins(gpio_num_t *tx_gpio, gpio_num_t *rx_gpio);

/**
 * Create the on-board TWAI node (esp_twai on-chip driver). The node is returned
 * disabled; the caller registers its callbacks and enables it.
 * A listen-only node leaves the TX pin unrouted so it cannot drive the bus.
 * Passing bitrate == 0 uses the default bitrate from esp_board_manager ("can" device) or 250000.
 */
esp_err_t board_twai_new_node(uint32_t bitrate, uint32_t tx_queue_depth, bool listen_only,
                              twai_node_handle_t *ret_node);

/** @brief Static UART configuration exposed from esp_board_manager. */
typedef struct {
    uart_port_t uart_num;
    gpio_num_t  tx_gpio;
    gpio_num_t  rx_gpio;
    gpio_num_t  rts_gpio;
    uart_mode_t uart_mode;
    int         baud_rate;
} board_uart_info_t;

/** @brief Get the RS-232 UART configuration from esp_board_manager. */
esp_err_t board_uart_232_get_info(board_uart_info_t *info);

/** @brief Get the RS-485 UART configuration from esp_board_manager. */
esp_err_t board_uart_485_get_info(board_uart_info_t *info);

/** @brief Initialize the RS-232 UART peripheral through esp_board_manager and return its port. */
esp_err_t board_uart_232_init(uart_port_t *uart_num);

/** @brief Release the RS-232 UART peripheral reference. */
esp_err_t board_uart_232_deinit(void);

/** @brief Initialize the RS-485 UART peripheral through esp_board_manager and return its port. */
esp_err_t board_uart_485_init(uart_port_t *uart_num);

/** @brief Release the RS-485 UART peripheral reference. */
esp_err_t board_uart_485_deinit(void);

/**
 * @brief Apply line format parameters to an initialized board UART port and flush RX.
 */
esp_err_t board_uart_configure(uart_port_t uart_num, int baud_rate,
                               uart_word_length_t data_bits, uart_parity_t parity,
                               uart_stop_bits_t stop_bits);

/** @brief Expansion header group for on-board broken-out GPIOs. */
typedef enum {
    BOARD_EXT_GPIO_HEADER_P1 = 0,          /*!< P1 16-pin expansion header */
    BOARD_EXT_GPIO_HEADER_P4,              /*!< P4 5-pin expansion header (unpopulated by default) */
} board_ext_gpio_header_t;

/** @brief Descriptor of an expansion header GPIO pin. */
typedef struct {
    gpio_num_t              gpio_num;
    board_ext_gpio_header_t header;
    uint8_t                 header_pin;
    const char             *name;
} board_ext_gpio_desc_t;

/**
 * @brief Get the list of P1/P4 expansion GPIOs on the board.
 *
 * @param[out] count Number of entries in the returned array (may be NULL)
 * @return Pointer to static array of expansion GPIO descriptors
 */
const board_ext_gpio_desc_t *board_ext_gpio_get_pins(size_t *count);

/** @brief Check whether a GPIO belongs to the P1/P4 expansion headers. */
bool board_ext_gpio_is_valid(gpio_num_t gpio_num);

/** @brief Configure an expansion GPIO on demand (does not touch unrequested pins). */
esp_err_t board_ext_gpio_config(gpio_num_t gpio_num, gpio_mode_t mode,
                                gpio_pullup_t pull_up, gpio_pulldown_t pull_down);

/** @brief Set output level on a configured expansion GPIO. */
esp_err_t board_ext_gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/** @brief Read input level from an expansion GPIO, or -1 if invalid. */
int board_ext_gpio_get_level(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif
