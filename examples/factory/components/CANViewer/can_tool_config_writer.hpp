#pragma once

#include <condition_variable>
#include <mutex>
#include "boost/thread.hpp"
#include "can_tool_config.hpp"

namespace esp_brookesia::apps {

class CanToolConfigWriter {
public:
    CanToolConfigWriter() = default;
    ~CanToolConfigWriter();

    CanToolConfigWriter(const CanToolConfigWriter &) = delete;
    CanToolConfigWriter &operator=(const CanToolConfigWriter &) = delete;

    bool start();
    void stop();
    bool requestSave(const CanToolConfig &config);
    bool takeFailure();

private:
    void threadLoop();

    std::mutex _mutex;
    std::condition_variable _cv;
    boost::thread _thread;
    CanToolConfig _pending_config;
    bool _running = false;
    bool _stop_requested = false;
    bool _pending = false;
    bool _last_save_failed = false;
};

} // namespace esp_brookesia::apps
