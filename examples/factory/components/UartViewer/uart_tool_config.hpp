#pragma once

#include <cstdint>
#include "board_peripherals.h"
#include "driver/gpio.h"
#include "driver/uart.h"

namespace esp_brookesia::apps {

enum class UartToolMode : int32_t {
    Uart = 0,
    Rs485,
    Rs232,
};

struct UartToolFixedPins {
    gpio_num_t tx;
    gpio_num_t rx;
    gpio_num_t direction;
};

// Transceiver routing comes from the board layer (esp_board_manager).
// Plain UART mode has no dedicated pins on this board, so it stays unavailable (NC).
inline UartToolFixedPins uart_tool_fixed_pins(UartToolMode mode)
{
    board_uart_info_t info = {};
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    switch (mode) {
    case UartToolMode::Rs485:
        ret = board_uart_485_get_info(&info);
        break;
    case UartToolMode::Rs232:
        ret = board_uart_232_get_info(&info);
        break;
    case UartToolMode::Uart:
    default:
        return {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    }
    if (ret != ESP_OK) {
        return {GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
    }
    return {
        info.tx_gpio,
        info.rx_gpio,
        (mode == UartToolMode::Rs485) ? info.rts_gpio : GPIO_NUM_NC,
    };
}

constexpr const char *uart_tool_mode_name(UartToolMode mode)
{
    switch (mode) {
    case UartToolMode::Rs485:
        return "RS485";
    case UartToolMode::Rs232:
        return "RS232";
    case UartToolMode::Uart:
    default:
        return "UART";
    }
}

inline bool uart_tool_fixed_pins_ready(UartToolMode mode)
{
    const UartToolFixedPins pins = uart_tool_fixed_pins(mode);
    return (pins.tx != GPIO_NUM_NC) &&
           (pins.rx != GPIO_NUM_NC) &&
           ((mode != UartToolMode::Rs485) || (pins.direction != GPIO_NUM_NC));
}

struct UartToolConfig {
    // Modes without dedicated pins (plain UART on this board) are not offered.
    UartToolMode mode = UartToolMode::Rs485;
    int baud = 115200;
    uart_word_length_t data_bits = UART_DATA_8_BITS;
    uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
    uart_parity_t parity = UART_PARITY_DISABLE;
    bool hex_rx = false;
    bool hex_tx = false;
};

UartToolConfig uart_tool_config_load();
bool uart_tool_config_save(const UartToolConfig &config);

} // namespace esp_brookesia::apps
