#pragma once

#include <cstdint>
#include "board_peripherals.h"
#include "driver/gpio.h"

namespace esp_brookesia::apps {

enum class CanOperatingMode : int32_t {
    Normal = 0,
    ListenOnly,
};

enum class CanFrameFilter : int32_t {
    All = 0,
    Standard,
    Extended,
};

// CAN transceiver routing comes from the board layer.
constexpr gpio_num_t CAN_TOOL_TX_GPIO = BOARD_TWAI_TX_GPIO;
constexpr gpio_num_t CAN_TOOL_RX_GPIO = BOARD_TWAI_RX_GPIO;

constexpr bool can_tool_pins_ready()
{
    return CAN_TOOL_TX_GPIO != GPIO_NUM_NC &&
           CAN_TOOL_RX_GPIO != GPIO_NUM_NC;
}

struct CanToolConfig {
    CanOperatingMode mode = CanOperatingMode::Normal;
    int bitrate = 500000;
    CanFrameFilter frame_filter = CanFrameFilter::All;
};

CanToolConfig can_tool_config_load();
bool can_tool_config_save(const CanToolConfig &config);

} // namespace esp_brookesia::apps
