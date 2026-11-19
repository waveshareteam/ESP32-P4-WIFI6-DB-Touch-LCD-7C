#include "SettingsStorage.hpp"

#include <algorithm>
#include "sdkconfig.h"
#include "esp_brookesia.hpp"

#define ESP_UTILS_LOG_TAG "BS:SettingsStorage"
#include "esp_lib_utils.h"

namespace esp_brookesia::apps::settings_storage {

namespace {

// NVS keys are limited to 15 characters.
constexpr char KEY_BRIGHTNESS[] = "brightness";
constexpr char KEY_VOLUME[] = "volume";
constexpr char KEY_THEME[] = "xz_theme";

void save(const char *key, int value);

int load(const char *key, int default_value, int min_value, int max_value)
{
    int value = default_value;
#if CONFIG_ESP_BROOKESIA_SERVICES_ENABLE_STORAGE_NVS
    services::StorageNVS::Value stored;
    if (services::StorageNVS::requestInstance().getLocalParam(key, stored) && std::holds_alternative<int>(stored)) {
        value = std::get<int>(stored);
    } else {
        // First boot: the key does not exist yet (the service logs a one-time
        // "not found" warning). Persist the default so later boots find it.
        save(key, std::clamp(default_value, min_value, max_value));
    }
#else
    (void)key;
#endif
    return std::clamp(value, min_value, max_value);
}

void save(const char *key, int value)
{
#if CONFIG_ESP_BROOKESIA_SERVICES_ENABLE_STORAGE_NVS
    // Asynchronous: the service writes NVS from its own thread.
    if (!services::StorageNVS::requestInstance().setLocalParam(key, value)) {
        ESP_UTILS_LOGW("Save %s failed", key);
    }
#else
    (void)key;
    (void)value;
#endif
}

} // namespace

int loadBrightness()
{
    return load(KEY_BRIGHTNESS, BRIGHTNESS_DEFAULT, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
}

void saveBrightness(int percent)
{
    save(KEY_BRIGHTNESS, std::clamp(percent, BRIGHTNESS_MIN, BRIGHTNESS_MAX));
}

int loadTheme()
{
    // TODO(theme): persistence disabled.
    // return load(KEY_THEME, THEME_DEFAULT, THEME_DARK, THEME_LIGHT);
    return THEME_DEFAULT;
}

void saveTheme(int theme)
{
    // TODO(theme): persistence disabled.
    (void)theme;
    // save(KEY_THEME, std::clamp(theme, THEME_DARK, THEME_LIGHT));
}

int loadVolume()
{
    return load(KEY_VOLUME, VOLUME_DEFAULT, VOLUME_MIN, VOLUME_MAX);
}

void saveVolume(int percent)
{
    save(KEY_VOLUME, std::clamp(percent, VOLUME_MIN, VOLUME_MAX));
}

} // namespace esp_brookesia::apps::settings_storage
