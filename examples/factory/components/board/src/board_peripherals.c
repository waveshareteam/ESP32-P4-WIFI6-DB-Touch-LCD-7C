/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board.h"
#include "board_peripherals.h"

#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "esp_twai_onchip.h"

#if __has_include("gen_board_device_custom.h")
#include "gen_board_device_custom.h"
#else
typedef struct {
    const char *name;
    const char *type;
    int8_t tx_io_num;
    int8_t rx_io_num;
    int32_t bitrate;
} dev_custom_can_config_t;
#endif

static const char *TAG = "board_periph";

#define BOARD_DEVICE_NAME_CAN      "can"
#define BOARD_PERIPH_NAME_UART_232 "uart_232"
#define BOARD_PERIPH_NAME_UART_485 "uart_485"

esp_err_t board_twai_get_pins(gpio_num_t *tx_gpio, gpio_num_t *rx_gpio)
{
    gpio_num_t tx = BOARD_TWAI_TX_GPIO;
    gpio_num_t rx = BOARD_TWAI_RX_GPIO;
    dev_custom_can_config_t *can_cfg = NULL;
    if (esp_board_manager_get_device_config(BOARD_DEVICE_NAME_CAN, (void **)&can_cfg) == ESP_OK && can_cfg) {
        tx = (gpio_num_t)can_cfg->tx_io_num;
        rx = (gpio_num_t)can_cfg->rx_io_num;
    }
    if (tx_gpio != NULL) {
        *tx_gpio = tx;
    }
    if (rx_gpio != NULL) {
        *rx_gpio = rx;
    }
    return ESP_OK;
}

esp_err_t board_twai_new_node(uint32_t bitrate, uint32_t tx_queue_depth, bool listen_only,
                              twai_node_handle_t *ret_node)
{
    ESP_RETURN_ON_FALSE(ret_node, ESP_ERR_INVALID_ARG, TAG, "Node handle is NULL");

    gpio_num_t tx_gpio = BOARD_TWAI_TX_GPIO;
    gpio_num_t rx_gpio = BOARD_TWAI_RX_GPIO;
    dev_custom_can_config_t *can_cfg = NULL;
    if (esp_board_manager_get_device_config(BOARD_DEVICE_NAME_CAN, (void **)&can_cfg) == ESP_OK && can_cfg) {
        tx_gpio = (gpio_num_t)can_cfg->tx_io_num;
        rx_gpio = (gpio_num_t)can_cfg->rx_io_num;
        if (bitrate == 0 && can_cfg->bitrate > 0) {
            bitrate = (uint32_t)can_cfg->bitrate;
        }
    }
    if (bitrate == 0) {
        bitrate = 250000;
    }

    twai_onchip_node_config_t config = {};
    config.io_cfg.tx = listen_only ? GPIO_NUM_NC : tx_gpio;
    config.io_cfg.rx = rx_gpio;
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

static esp_err_t board_uart_get_info_by_name(const char *periph_name, board_uart_info_t *info)
{
    ESP_RETURN_ON_FALSE(info, ESP_ERR_INVALID_ARG, TAG, "UART info pointer is NULL");

    periph_uart_config_t *uart_cfg = NULL;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_periph_config(periph_name, (void **)&uart_cfg),
                        TAG, "Get %s config failed", periph_name);
    ESP_RETURN_ON_FALSE(uart_cfg, ESP_ERR_INVALID_STATE, TAG, "Invalid %s config", periph_name);

    *info = (board_uart_info_t) {
        .uart_num = uart_cfg->uart_num,
        .tx_gpio = uart_cfg->tx_io_num,
        .rx_gpio = uart_cfg->rx_io_num,
        .rts_gpio = uart_cfg->rts_io_num,
        .uart_mode = uart_cfg->uart_mode,
        .baud_rate = uart_cfg->uart_config.baud_rate,
    };
    return ESP_OK;
}

esp_err_t board_uart_232_get_info(board_uart_info_t *info)
{
    return board_uart_get_info_by_name(BOARD_PERIPH_NAME_UART_232, info);
}

esp_err_t board_uart_485_get_info(board_uart_info_t *info)
{
    return board_uart_get_info_by_name(BOARD_PERIPH_NAME_UART_485, info);
}

static esp_err_t board_uart_init_by_name(const char *periph_name, uart_port_t *uart_num)
{
    periph_uart_handle_t *handle = NULL;
    ESP_RETURN_ON_ERROR(esp_board_periph_ref_handle(periph_name, (void **)&handle),
                        TAG, "Initialize %s failed", periph_name);
    if (handle == NULL) {
        esp_board_periph_unref_handle(periph_name);
        return ESP_ERR_INVALID_STATE;
    }
    if (uart_num != NULL) {
        *uart_num = handle->uart_num;
    }
    return ESP_OK;
}

esp_err_t board_uart_232_init(uart_port_t *uart_num)
{
    return board_uart_init_by_name(BOARD_PERIPH_NAME_UART_232, uart_num);
}

esp_err_t board_uart_232_deinit(void)
{
    return esp_board_periph_unref_handle(BOARD_PERIPH_NAME_UART_232);
}

esp_err_t board_uart_485_init(uart_port_t *uart_num)
{
    return board_uart_init_by_name(BOARD_PERIPH_NAME_UART_485, uart_num);
}

esp_err_t board_uart_485_deinit(void)
{
    return esp_board_periph_unref_handle(BOARD_PERIPH_NAME_UART_485);
}

esp_err_t board_uart_configure(uart_port_t uart_num, int baud_rate,
                               uart_word_length_t data_bits, uart_parity_t parity,
                               uart_stop_bits_t stop_bits)
{
    ESP_RETURN_ON_ERROR(uart_set_baudrate(uart_num, (uint32_t)baud_rate), TAG, "Set UART baudrate failed");
    ESP_RETURN_ON_ERROR(uart_set_word_length(uart_num, data_bits), TAG, "Set UART word length failed");
    ESP_RETURN_ON_ERROR(uart_set_parity(uart_num, parity), TAG, "Set UART parity failed");
    ESP_RETURN_ON_ERROR(uart_set_stop_bits(uart_num, stop_bits), TAG, "Set UART stop bits failed");
    return uart_flush_input(uart_num);
}

esp_err_t board_get_info(board_info_t *info)
{
    ESP_RETURN_ON_FALSE(info, ESP_ERR_INVALID_ARG, TAG, "Board info pointer is NULL");

    esp_board_info_t bmgr_info = {};
    ESP_RETURN_ON_ERROR(esp_board_manager_get_board_info(&bmgr_info), TAG, "Get board info failed");
    *info = (board_info_t) {
        .name = bmgr_info.name,
        .chip = bmgr_info.chip,
        .version = bmgr_info.version,
        .description = bmgr_info.description,
        .manufacturer = bmgr_info.manufacturer,
    };
    return ESP_OK;
}

static const board_ext_gpio_desc_t s_ext_gpios[] = {
    { .gpio_num = GPIO_NUM_2,  .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 14, .name = "GPIO2" },
    { .gpio_num = GPIO_NUM_3,  .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 13, .name = "GPIO3" },
    { .gpio_num = GPIO_NUM_4,  .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 12, .name = "GPIO4" },
    { .gpio_num = GPIO_NUM_5,  .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 11, .name = "GPIO5" },
    { .gpio_num = GPIO_NUM_36, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 8,  .name = "GPIO36" },
    { .gpio_num = GPIO_NUM_51, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 5,  .name = "GPIO51" },
    { .gpio_num = GPIO_NUM_32, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 10, .name = "GPIO32" },
    { .gpio_num = GPIO_NUM_33, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 9,  .name = "GPIO33" },
    { .gpio_num = GPIO_NUM_49, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 7,  .name = "GPIO49" },
    { .gpio_num = GPIO_NUM_50, .header = BOARD_EXT_GPIO_HEADER_P1, .header_pin = 6,  .name = "GPIO50" },
    { .gpio_num = GPIO_NUM_46, .header = BOARD_EXT_GPIO_HEADER_P4, .header_pin = 3,  .name = "GPIO46" },
    { .gpio_num = GPIO_NUM_47, .header = BOARD_EXT_GPIO_HEADER_P4, .header_pin = 2,  .name = "GPIO47" },
    { .gpio_num = GPIO_NUM_48, .header = BOARD_EXT_GPIO_HEADER_P4, .header_pin = 4,  .name = "GPIO48" },
};

const board_ext_gpio_desc_t *board_ext_gpio_get_pins(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(s_ext_gpios) / sizeof(s_ext_gpios[0]);
    }
    return s_ext_gpios;
}

bool board_ext_gpio_is_valid(gpio_num_t gpio_num)
{
    for (size_t i = 0; i < sizeof(s_ext_gpios) / sizeof(s_ext_gpios[0]); i++) {
        if (s_ext_gpios[i].gpio_num == gpio_num) {
            return true;
        }
    }
    return false;
}

esp_err_t board_ext_gpio_config(gpio_num_t gpio_num, gpio_mode_t mode,
                                gpio_pullup_t pull_up, gpio_pulldown_t pull_down)
{
    ESP_RETURN_ON_FALSE(board_ext_gpio_is_valid(gpio_num), ESP_ERR_INVALID_ARG,
                        TAG, "GPIO %d is not an expansion pin", (int)gpio_num);

    const gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << (uint32_t)gpio_num,
        .mode = mode,
        .pull_up_en = pull_up,
        .pull_down_en = pull_down,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&cfg);
}

esp_err_t board_ext_gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    ESP_RETURN_ON_FALSE(board_ext_gpio_is_valid(gpio_num), ESP_ERR_INVALID_ARG,
                        TAG, "GPIO %d is not an expansion pin", (int)gpio_num);
    return gpio_set_level(gpio_num, level);
}

int board_ext_gpio_get_level(gpio_num_t gpio_num)
{
    if (!board_ext_gpio_is_valid(gpio_num)) {
        return -1;
    }
    return gpio_get_level(gpio_num);
}
