#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:CanTool"
#include "esp_lib_utils.h"

#include <inttypes.h>
#include "nvs.h"
#include "can_tool_config.hpp"

#define NVS_NAMESPACE "can_tool"

namespace esp_brookesia::apps {

namespace {

bool is_supported_mode(int32_t mode)
{
    return mode >= static_cast<int32_t>(CanOperatingMode::Normal) &&
           mode <= static_cast<int32_t>(CanOperatingMode::ListenOnly);
}

bool is_supported_filter(int32_t filter)
{
    return filter >= static_cast<int32_t>(CanFrameFilter::All) &&
           filter <= static_cast<int32_t>(CanFrameFilter::Extended);
}

bool is_supported_bitrate(int32_t bitrate)
{
    static const int32_t supported[] = {125000, 250000, 500000, 1000000};
    for (int32_t value : supported) {
        if (bitrate == value) {
            return true;
        }
    }
    return false;
}

void load_i32(nvs_handle_t handle, const char *key, int32_t &value)
{
    int32_t stored = 0;
    if (nvs_get_i32(handle, key, &stored) == ESP_OK) {
        value = stored;
    }
}

} // namespace

CanToolConfig can_tool_config_load()
{
    CanToolConfig config;

    nvs_handle_t handle;
    const esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_UTILS_LOGW("NVS open failed (%s), use defaults", esp_err_to_name(err));
        return config;
    }

    int32_t mode = static_cast<int32_t>(config.mode);
    int32_t bitrate = config.bitrate;
    int32_t frame_filter = static_cast<int32_t>(config.frame_filter);
    load_i32(handle, "mode", mode);
    load_i32(handle, "bitrate", bitrate);
    load_i32(handle, "filter", frame_filter);
    nvs_close(handle);

    bool corrected = false;
    const CanToolConfig defaults;
    if (!is_supported_mode(mode)) {
        ESP_UTILS_LOGW(
            "Invalid NVS mode=%" PRId32 ", use default %" PRId32,
            mode, static_cast<int32_t>(defaults.mode)
        );
        mode = static_cast<int32_t>(defaults.mode);
        corrected = true;
    }
    if (!is_supported_bitrate(bitrate)) {
        ESP_UTILS_LOGW(
            "Invalid NVS bitrate=%" PRId32 ", use default %" PRId32,
            bitrate, defaults.bitrate
        );
        bitrate = defaults.bitrate;
        corrected = true;
    }
    if (!is_supported_filter(frame_filter)) {
        ESP_UTILS_LOGW(
            "Invalid NVS filter=%" PRId32 ", use default %" PRId32,
            frame_filter, static_cast<int32_t>(defaults.frame_filter)
        );
        frame_filter = static_cast<int32_t>(defaults.frame_filter);
        corrected = true;
    }

    config.mode = static_cast<CanOperatingMode>(mode);
    config.bitrate = bitrate;
    config.frame_filter = static_cast<CanFrameFilter>(frame_filter);
    if (corrected) {
        can_tool_config_save(config);
    }
    return config;
}

bool can_tool_config_save(const CanToolConfig &config)
{
    nvs_handle_t handle;
    const esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    ESP_UTILS_CHECK_FALSE_RETURN(
        err == ESP_OK, false, "NVS open failed: %s", esp_err_to_name(err)
    );

    bool ok = true;
    ok &= (nvs_set_i32(handle, "mode", static_cast<int32_t>(config.mode)) == ESP_OK);
    ok &= (nvs_set_i32(handle, "bitrate", config.bitrate) == ESP_OK);
    ok &= (nvs_set_i32(
        handle, "filter", static_cast<int32_t>(config.frame_filter)
    ) == ESP_OK);
    ok &= (nvs_commit(handle) == ESP_OK);
    nvs_close(handle);

    ESP_UTILS_CHECK_FALSE_RETURN(ok, false, "NVS write failed");
    return true;
}

} // namespace esp_brookesia::apps
