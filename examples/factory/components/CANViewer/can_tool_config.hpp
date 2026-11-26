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

struct CanToolPins {
    gpio_num_t tx;
    gpio_num_t rx;
};

// CAN transceiver routing comes from the board layer at runtime.
inline CanToolPins can_tool_get_pins()
{
    gpio_num_t tx = GPIO_NUM_NC;
    gpio_num_t rx = GPIO_NUM_NC;
    if (board_twai_get_pins(&tx, &rx) != ESP_OK) {
        return {GPIO_NUM_NC, GPIO_NUM_NC};
    }
    return {tx, rx};
}

inline bool can_tool_pins_ready()
{
    const CanToolPins pins = can_tool_get_pins();
    return pins.tx != GPIO_NUM_NC &&
           pins.rx != GPIO_NUM_NC;
}

struct CanToolConfig {
    CanOperatingMode mode = CanOperatingMode::Normal;
    int bitrate = 500000;
    CanFrameFilter frame_filter = CanFrameFilter::All;
};

CanToolConfig can_tool_config_load();
bool can_tool_config_save(const CanToolConfig &config);

} // namespace esp_brookesia::apps
