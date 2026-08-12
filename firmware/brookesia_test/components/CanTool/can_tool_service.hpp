#pragma once

#include <cstddef>
#include <cstdint>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_twai.h"

namespace esp_brookesia::apps {

enum class CanToolBusState : uint8_t {
    Stopped = 0,
    Active,
    Warning,
    Passive,
    BusOff,
};

struct CanToolFrame {
    uint64_t timestamp_us = 0;
    uint32_t id = 0;
    uint8_t data[8] = {};
    uint8_t dlc = 0;
    bool extended = false;
    bool remote = false;
    bool tx = false;
};

struct CanToolServiceStatus {
    bool running = false;
    bool listen_only = false;
    CanToolBusState state = CanToolBusState::Stopped;
    uint16_t tx_error_count = 0;
    uint16_t rx_error_count = 0;
    uint64_t tx_count = 0;
    uint64_t rx_count = 0;
    uint64_t dropped_count = 0;
    uint64_t tx_failed_count = 0;
    uint32_t bus_error_count = 0;
    uint32_t last_error_flags = 0;
    esp_err_t last_error = ESP_OK;
};

class CanToolService {
public:
    CanToolService() = default;
    ~CanToolService();

    CanToolService(const CanToolService &) = delete;
    CanToolService &operator=(const CanToolService &) = delete;

    esp_err_t start(
        gpio_num_t tx_gpio,
        gpio_num_t rx_gpio,
        int bitrate,
        bool listen_only
    );
    esp_err_t stop();

    esp_err_t send(
        uint32_t id,
        bool extended,
        bool remote,
        const uint8_t *data,
        uint8_t dlc
    );

    size_t readFrames(CanToolFrame *out, size_t max_frames);
    void clear();
    CanToolServiceStatus getStatus() const;

    // start(), stop(), send(), clear() and getStatus() are called by the LVGL
    // task. Driver callbacks only touch a fixed-capacity internal-SRAM Context
    // under its short critical section.

private:
    struct Context;

    static bool IRAM_ATTR onRxDone(
        twai_node_handle_t handle,
        const twai_rx_done_event_data_t *event_data,
        void *user_ctx
    );
    static bool IRAM_ATTR onTxDone(
        twai_node_handle_t handle,
        const twai_tx_done_event_data_t *event_data,
        void *user_ctx
    );
    static bool IRAM_ATTR onStateChange(
        twai_node_handle_t handle,
        const twai_state_change_event_data_t *event_data,
        void *user_ctx
    );
    static bool IRAM_ATTR onError(
        twai_node_handle_t handle,
        const twai_error_event_data_t *event_data,
        void *user_ctx
    );

    static void IRAM_ATTR pushFrameLocked(
        Context *context, const CanToolFrame &frame
    );
    static CanToolBusState IRAM_ATTR mapState(twai_error_state_t state);
    static void freeContext(Context *context);

    twai_node_handle_t _node = nullptr;
    Context *_context = nullptr;
    bool _running = false;
    bool _listen_only = false;
    bool _stopping = false;
    esp_err_t _last_error = ESP_OK;
};

} // namespace esp_brookesia::apps
