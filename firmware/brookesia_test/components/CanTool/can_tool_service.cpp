#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:CanTool"
#include "esp_lib_utils.h"

#include <algorithm>
#include <new>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_twai_onchip.h"
#include "freertos/FreeRTOS.h"
#include "hal/twai_types.h"
#include "can_tool_service.hpp"

namespace esp_brookesia::apps {

namespace {

constexpr size_t RX_RING_SIZE = 256;
constexpr size_t TX_SLOT_COUNT = 8;

} // namespace

struct CanToolService::Context {
    struct TxSlot {
        twai_frame_t driver_frame = {};
        CanToolFrame public_frame = {};
        bool in_use = false;
    };

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    CanToolFrame rx_ring[RX_RING_SIZE] = {};
    size_t rx_head = 0;
    size_t rx_tail = 0;
    size_t rx_size = 0;
    TxSlot tx_slots[TX_SLOT_COUNT] = {};

    CanToolBusState state = CanToolBusState::Stopped;
    uint64_t tx_count = 0;
    uint64_t rx_count = 0;
    uint64_t dropped_count = 0;
    uint64_t tx_failed_count = 0;
    uint32_t bus_error_count = 0;
    uint32_t last_error_flags = 0;
};

CanToolService::~CanToolService()
{
    stop();
}

esp_err_t CanToolService::start(
    gpio_num_t tx_gpio,
    gpio_num_t rx_gpio,
    int bitrate,
    bool listen_only
)
{
    const esp_err_t stop_result = stop();
    if (stop_result != ESP_OK) {
        return stop_result;
    }
    if ((!listen_only && tx_gpio == GPIO_NUM_NC) ||
        rx_gpio == GPIO_NUM_NC || bitrate <= 0) {
        _last_error = ESP_ERR_INVALID_ARG;
        return ESP_ERR_INVALID_ARG;
    }

    void *memory = heap_caps_calloc(
        1, sizeof(Context), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );
    if (memory == nullptr) {
        _last_error = ESP_ERR_NO_MEM;
        ESP_UTILS_LOGE("Allocate internal TWAI context failed");
        return ESP_ERR_NO_MEM;
    }
    auto *context = new (memory) Context();

    twai_onchip_node_config_t node_config = {};
    // A listen-only node must not drive the transceiver TX input. Keep only
    // RX routed, matching the ESP-IDF listen-only reference implementation.
    node_config.io_cfg.tx = listen_only ? GPIO_NUM_NC : tx_gpio;
    node_config.io_cfg.rx = rx_gpio;
    node_config.io_cfg.quanta_clk_out = GPIO_NUM_NC;
    node_config.io_cfg.bus_off_indicator = GPIO_NUM_NC;
    node_config.bit_timing.bitrate = static_cast<uint32_t>(bitrate);
    node_config.timestamp_resolution_hz = 1000000;
    // A bounded retry count prevents one missing ACK from retaining a TX slot
    // forever while preserving normal arbitration retries.
    node_config.fail_retry_cnt = 3;
    node_config.tx_queue_depth = TX_SLOT_COUNT;
    node_config.flags.enable_listen_only = listen_only;

    twai_node_handle_t node = nullptr;
    esp_err_t err = twai_new_node_onchip(&node_config, &node);
    if (err != ESP_OK) {
        freeContext(context);
        _last_error = err;
        ESP_UTILS_LOGE("Create TWAI node failed: %s", esp_err_to_name(err));
        return err;
    }

    // The UI applies STD/EXT filtering to its history. Keep the hardware
    // filter explicitly open so listen-only mode receives both frame formats.
    const twai_mask_filter_config_t receive_all_filter = {};
    err = twai_node_config_mask_filter(node, 0, &receive_all_filter);

    const twai_event_callbacks_t callbacks = {
        .on_tx_done = onTxDone,
        .on_rx_done = onRxDone,
        .on_state_change = onStateChange,
        .on_error = onError,
    };
    if (err == ESP_OK) {
        err = twai_node_register_event_callbacks(node, &callbacks, context);
    }
    if (err == ESP_OK) {
        err = twai_node_enable(node);
    }
    if (err != ESP_OK) {
        esp_err_t cleanup_err = twai_node_disable(node);
        if (cleanup_err == ESP_ERR_INVALID_STATE) {
            cleanup_err = ESP_OK;
        }
        if (cleanup_err == ESP_OK) {
            cleanup_err = twai_node_delete(node);
        }
        if (cleanup_err == ESP_OK) {
            freeContext(context);
        } else {
            // Preserve both objects so a later stop() can retry cleanup. Never
            // free callback user data while the driver still owns the node.
            _node = node;
            _context = context;
            _running = false;
            _listen_only = listen_only;
        }
        _last_error = err;
        ESP_UTILS_LOGE("Start TWAI node failed: %s", esp_err_to_name(err));
        if (cleanup_err != ESP_OK) {
            ESP_UTILS_LOGE(
                "TWAI start cleanup failed: %s", esp_err_to_name(cleanup_err)
            );
        }
        return err;
    }

    twai_node_status_t initial_status = {};
    if (twai_node_get_info(node, &initial_status, nullptr) == ESP_OK) {
        portENTER_CRITICAL(&context->mux);
        context->state = mapState(initial_status.state);
        portEXIT_CRITICAL(&context->mux);
    }

    _node = node;
    _context = context;
    _running = true;
    _listen_only = listen_only;
    _stopping = false;
    _last_error = ESP_OK;

    ESP_UTILS_LOGI(
        "TWAI started: tx=%d rx=%d bitrate=%d mode=%s",
        static_cast<int>(node_config.io_cfg.tx),
        static_cast<int>(rx_gpio),
        bitrate,
        listen_only ? "listen-only" : "normal"
    );
    return ESP_OK;
}

esp_err_t CanToolService::stop()
{
    if (_node == nullptr) {
        if (_context != nullptr) {
            freeContext(_context);
            _context = nullptr;
        }
        _running = false;
        _listen_only = false;
        _stopping = false;
        return ESP_OK;
    }

    _stopping = true;
    esp_err_t result = twai_node_disable(_node);
    if (result == ESP_ERR_INVALID_STATE) {
        // The v2 driver represents both a stopped node and a real bus-off node
        // with TWAI_ERROR_BUS_OFF. Both are already eligible for delete().
        result = ESP_OK;
    }
    const bool node_stopped = result == ESP_OK;
    if (result == ESP_OK) {
        result = twai_node_delete(_node);
    }

    _last_error = result;
    if (result == ESP_OK) {
        _node = nullptr;
        freeContext(_context);
        _context = nullptr;
        _running = false;
        _listen_only = false;
        _stopping = false;
        ESP_UTILS_LOGI("TWAI stopped");
    } else {
        if (node_stopped) {
            _running = false;
        }
        _stopping = false;
        ESP_UTILS_LOGE("Stop TWAI failed: %s", esp_err_to_name(result));
    }
    return result;
}

esp_err_t CanToolService::send(
    uint32_t id,
    bool extended,
    bool remote,
    const uint8_t *data,
    uint8_t dlc
)
{
    const uint32_t id_limit = extended ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK;
    if (id > id_limit || dlc > TWAI_FRAME_MAX_DLC ||
        (!remote && dlc > 0 && data == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!_running || _stopping || _listen_only ||
        _node == nullptr || _context == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    Context::TxSlot *slot = nullptr;
    portENTER_CRITICAL(&_context->mux);
    for (auto &candidate : _context->tx_slots) {
        if (!candidate.in_use) {
            slot = &candidate;
            break;
        }
    }
    if (slot == nullptr) {
        _context->tx_failed_count++;
        portEXIT_CRITICAL(&_context->mux);
        return ESP_ERR_TIMEOUT;
    }

    slot->in_use = true;
    portEXIT_CRITICAL(&_context->mux);

    slot->public_frame = {};
    slot->public_frame.id = id;
    slot->public_frame.dlc = dlc;
    slot->public_frame.extended = extended;
    slot->public_frame.remote = remote;
    slot->public_frame.tx = true;
    for (uint8_t i = 0; i < dlc; i++) {
        slot->public_frame.data[i] = remote ? 0 : data[i];
    }

    slot->driver_frame = {};
    slot->driver_frame.header.id = id;
    slot->driver_frame.header.dlc = dlc;
    slot->driver_frame.header.ide = extended;
    slot->driver_frame.header.rtr = remote;
    slot->driver_frame.buffer = slot->public_frame.data;
    slot->driver_frame.buffer_len = remote ? 0 : dlc;

    const esp_err_t err = twai_node_transmit(_node, &slot->driver_frame, 0);
    if (err != ESP_OK) {
        portENTER_CRITICAL(&_context->mux);
        slot->in_use = false;
        _context->tx_failed_count++;
        portEXIT_CRITICAL(&_context->mux);
        _last_error = err;
    }
    return err;
}

size_t CanToolService::readFrames(CanToolFrame *out, size_t max_frames)
{
    if (out == nullptr || max_frames == 0 || _context == nullptr) {
        return 0;
    }

    portENTER_CRITICAL(&_context->mux);
    const size_t count = std::min(max_frames, _context->rx_size);
    for (size_t i = 0; i < count; i++) {
        out[i] = _context->rx_ring[_context->rx_tail];
        _context->rx_tail = (_context->rx_tail + 1) % RX_RING_SIZE;
    }
    _context->rx_size -= count;
    portEXIT_CRITICAL(&_context->mux);
    return count;
}

void CanToolService::clear()
{
    if (_context == nullptr) {
        return;
    }
    portENTER_CRITICAL(&_context->mux);
    _context->rx_head = 0;
    _context->rx_tail = 0;
    _context->rx_size = 0;
    _context->tx_count = 0;
    _context->rx_count = 0;
    _context->dropped_count = 0;
    _context->tx_failed_count = 0;
    _context->bus_error_count = 0;
    _context->last_error_flags = 0;
    portEXIT_CRITICAL(&_context->mux);
}

CanToolServiceStatus CanToolService::getStatus() const
{
    CanToolServiceStatus status;
    status.running = _running;
    status.listen_only = _listen_only;
    status.last_error = _last_error;

    if (_context != nullptr) {
        portENTER_CRITICAL(&_context->mux);
        status.state = _context->state;
        status.tx_count = _context->tx_count;
        status.rx_count = _context->rx_count;
        status.dropped_count = _context->dropped_count;
        status.tx_failed_count = _context->tx_failed_count;
        status.bus_error_count = _context->bus_error_count;
        status.last_error_flags = _context->last_error_flags;
        portEXIT_CRITICAL(&_context->mux);
    }

    if (status.running && _node != nullptr) {
        twai_node_status_t node_status = {};
        twai_node_record_t node_record = {};
        const esp_err_t err = twai_node_get_info(
            _node, &node_status, &node_record
        );
        if (err == ESP_OK) {
            status.state = mapState(node_status.state);
            status.tx_error_count = node_status.tx_error_count;
            status.rx_error_count = node_status.rx_error_count;
            status.bus_error_count = std::max(
                status.bus_error_count, node_record.bus_err_num
            );
        } else {
            status.last_error = err;
        }
    }
    return status;
}

bool IRAM_ATTR CanToolService::onRxDone(
    twai_node_handle_t handle,
    const twai_rx_done_event_data_t *,
    void *user_ctx
)
{
    auto *context = static_cast<Context *>(user_ctx);
    if (context == nullptr) {
        return false;
    }

    uint8_t data[8] = {};
    twai_frame_t driver_frame = {};
    driver_frame.buffer = data;
    driver_frame.buffer_len = sizeof(data);
    if (twai_node_receive_from_isr(handle, &driver_frame) != ESP_OK) {
        return false;
    }

    CanToolFrame frame;
    frame.timestamp_us = driver_frame.header.timestamp;
    frame.id = driver_frame.header.id;
    frame.dlc = static_cast<uint8_t>(
        driver_frame.header.dlc > TWAI_FRAME_MAX_DLC ?
        TWAI_FRAME_MAX_DLC : driver_frame.header.dlc
    );
    frame.extended = driver_frame.header.ide;
    frame.remote = driver_frame.header.rtr;
    frame.tx = false;
    if (!frame.remote) {
        for (uint8_t i = 0; i < frame.dlc; i++) {
            frame.data[i] = data[i];
        }
    }

    portENTER_CRITICAL_ISR(&context->mux);
    context->rx_count++;
    pushFrameLocked(context, frame);
    portEXIT_CRITICAL_ISR(&context->mux);
    return false;
}

bool IRAM_ATTR CanToolService::onTxDone(
    twai_node_handle_t,
    const twai_tx_done_event_data_t *event_data,
    void *user_ctx
)
{
    auto *context = static_cast<Context *>(user_ctx);
    if (context == nullptr || event_data == nullptr ||
        event_data->done_tx_frame == nullptr) {
        return false;
    }

    CanToolFrame completed_frame;
    const uint64_t completion_time_us =
        static_cast<uint64_t>(esp_timer_get_time());
    portENTER_CRITICAL_ISR(&context->mux);
    for (auto &slot : context->tx_slots) {
        if (&slot.driver_frame != event_data->done_tx_frame) {
            continue;
        }
        if (event_data->is_tx_success) {
            completed_frame = slot.public_frame;
            completed_frame.timestamp_us = completion_time_us;
            context->tx_count++;
            pushFrameLocked(context, completed_frame);
        } else {
            context->tx_failed_count++;
        }
        slot.in_use = false;
        break;
    }
    portEXIT_CRITICAL_ISR(&context->mux);
    return false;
}

bool IRAM_ATTR CanToolService::onStateChange(
    twai_node_handle_t,
    const twai_state_change_event_data_t *event_data,
    void *user_ctx
)
{
    auto *context = static_cast<Context *>(user_ctx);
    if (context == nullptr || event_data == nullptr) {
        return false;
    }
    portENTER_CRITICAL_ISR(&context->mux);
    context->state = mapState(event_data->new_sta);
    portEXIT_CRITICAL_ISR(&context->mux);
    return false;
}

bool IRAM_ATTR CanToolService::onError(
    twai_node_handle_t,
    const twai_error_event_data_t *event_data,
    void *user_ctx
)
{
    auto *context = static_cast<Context *>(user_ctx);
    if (context == nullptr || event_data == nullptr) {
        return false;
    }
    portENTER_CRITICAL_ISR(&context->mux);
    context->bus_error_count++;
    context->last_error_flags = event_data->err_flags.val;
    portEXIT_CRITICAL_ISR(&context->mux);
    return false;
}

void IRAM_ATTR CanToolService::pushFrameLocked(
    Context *context, const CanToolFrame &frame
)
{
    if (context->rx_size == RX_RING_SIZE) {
        context->rx_tail = (context->rx_tail + 1) % RX_RING_SIZE;
        context->rx_size--;
        context->dropped_count++;
    }
    context->rx_ring[context->rx_head] = frame;
    context->rx_head = (context->rx_head + 1) % RX_RING_SIZE;
    context->rx_size++;
}

CanToolBusState IRAM_ATTR CanToolService::mapState(twai_error_state_t state)
{
    switch (state) {
    case TWAI_ERROR_ACTIVE:
        return CanToolBusState::Active;
    case TWAI_ERROR_WARNING:
        return CanToolBusState::Warning;
    case TWAI_ERROR_PASSIVE:
        return CanToolBusState::Passive;
    case TWAI_ERROR_BUS_OFF:
        return CanToolBusState::BusOff;
    default:
        return CanToolBusState::Stopped;
    }
}

void CanToolService::freeContext(Context *context)
{
    if (context == nullptr) {
        return;
    }
    context->~Context();
    heap_caps_free(context);
}

} // namespace esp_brookesia::apps
