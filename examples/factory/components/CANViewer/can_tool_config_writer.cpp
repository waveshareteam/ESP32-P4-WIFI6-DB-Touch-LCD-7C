#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:CanTool"
#include "esp_lib_utils.h"

#include <exception>
#include "can_tool_config_writer.hpp"

namespace esp_brookesia::apps {

namespace {

constexpr size_t CONFIG_WRITER_STACK_SIZE = 4 * 1024;

} // namespace

CanToolConfigWriter::~CanToolConfigWriter()
{
    stop();
}

bool CanToolConfigWriter::start()
{
    stop();
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stop_requested = false;
        _pending = false;
        _last_save_failed = false;
        _running = true;
    }

    try {
        esp_utils::thread_config_guard thread_config({
            .name = "can_cfg",
            .stack_size = CONFIG_WRITER_STACK_SIZE,
        });
        _thread = boost::thread([this]() {
            threadLoop();
        });
    } catch (const std::exception &error) {
        std::lock_guard<std::mutex> lock(_mutex);
        _running = false;
        ESP_UTILS_LOGE("Start config writer failed: %s", error.what());
        return false;
    }
    return true;
}

void CanToolConfigWriter::stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running) {
            return;
        }
        _stop_requested = true;
        _cv.notify_one();
    }
    if (_thread.joinable()) {
        _thread.join();
    }
    std::lock_guard<std::mutex> lock(_mutex);
    _running = false;
    _stop_requested = false;
    _pending = false;
}

bool CanToolConfigWriter::requestSave(const CanToolConfig &config)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running && !_stop_requested) {
            // One bounded pending slot coalesces rapid mode/filter changes.
            // The NVS writer never owns or touches LVGL objects.
            _pending_config = config;
            _pending = true;
            _cv.notify_one();
            return true;
        }
    }

    if (!start()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _pending_config = config;
    _pending = true;
    _cv.notify_one();
    return true;
}

bool CanToolConfigWriter::takeFailure()
{
    std::lock_guard<std::mutex> lock(_mutex);
    const bool failed = _last_save_failed;
    _last_save_failed = false;
    return failed;
}

void CanToolConfigWriter::threadLoop()
{
    while (true) {
        CanToolConfig config;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() {
                return _stop_requested || _pending;
            });
            if (!_pending && _stop_requested) {
                break;
            }
            config = _pending_config;
            _pending = false;
        }

        const bool saved = can_tool_config_save(config);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!saved) {
                _last_save_failed = true;
            }
            if (_stop_requested && !_pending) {
                break;
            }
        }
    }
}

} // namespace esp_brookesia::apps
