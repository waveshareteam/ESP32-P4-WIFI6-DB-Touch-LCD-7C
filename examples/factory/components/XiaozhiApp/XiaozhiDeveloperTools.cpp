/* SPDX-License-Identifier: Apache-2.0 */

#include "XiaozhiDeveloperTools.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "Camera.hpp"
#include "SettingsStorage.hpp"
#if XIAOZHI_THEME_ENABLED
#include "XiaozhiAssets.hpp"
#endif
#include "board.h"
#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_mcp_engine.h"
#include "esp_mcp_tool.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_xiaozhi_camera.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace esp_brookesia::apps {
namespace {
// Leave time for the tool reply and the spoken confirmation before the restart.
constexpr uint64_t REBOOT_DELAY_US = 3 * 1000 * 1000;

std::mutex camera_mutex;
esp_xiaozhi_chat_handle_t camera_chat = 0;
std::mutex reboot_mutex;
esp_timer_handle_t reboot_timer = nullptr;

esp_mcp_value_t explainImage(const esp_mcp_property_list_t *properties)
{
    std::lock_guard<std::mutex> lock(camera_mutex);
    if (!camera_chat) return esp_mcp_value_create_string("Camera unavailable: chat is stopped");
    const char *question = esp_mcp_property_list_get_property_string(properties, "question");
    if (!question || !question[0]) return esp_mcp_value_create_string("A question is required");
    char url[256] = {};
    char token[128] = {};
    esp_err_t ret = esp_xiaozhi_chat_get_vision_explain(camera_chat, url, sizeof(url), token, sizeof(token));
    if (ret != ESP_OK) return esp_mcp_value_create_string("Vision service is not available");
    uint8_t *jpeg = nullptr;
    size_t jpeg_len = 0;
    Camera::CapturePreview preview;
    ret = Camera::requestInstance()->captureJpeg(&jpeg, &jpeg_len, &preview);
    if (ret != ESP_OK) return esp_mcp_value_create_string("Camera capture failed or camera is busy");
    // Show the photo on screen while it is uploaded and described.
    Camera::showCapturePreview(preview);
    esp_xiaozhi_camera_config_t config = {};
    config.explain_url = url;
    config.explain_token = token;
    esp_xiaozhi_camera_handle_t *camera = nullptr;
    ret = esp_xiaozhi_camera_create(&config, &camera);
    char *response = static_cast<char *>(heap_caps_calloc(1, 4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!response && ret == ESP_OK) ret = ESP_ERR_NO_MEM;
    if (ret == ESP_OK) {
        esp_xiaozhi_camera_frame_t frame = {};
        frame.data = jpeg;
        frame.len = jpeg_len;
        ret = esp_xiaozhi_camera_explain(camera, &frame, question, response, 4096, nullptr);
    }
    if (camera) esp_xiaozhi_camera_destroy(camera);
    free(jpeg);
    if (ret != ESP_OK) {
        free(response);
        return esp_mcp_value_create_string("Vision upload failed");
    }
    esp_mcp_value_t result = esp_mcp_value_create_string(response);
    free(response);
    return result;
}

esp_mcp_value_t jsonValue(cJSON *root)
{
    char *text = root ? cJSON_PrintUnformatted(root) : nullptr;
    cJSON_Delete(root);
    if (!text) return esp_mcp_value_create_string("{}");
    esp_mcp_value_t result = esp_mcp_value_create_string(text);
    cJSON_free(text);
    return result;
}

// Volume and brightness use the Settings ranges and are saved, so they survive a reboot like Settings changes.
esp_mcp_value_t setVolume(const esp_mcp_property_list_t *properties)
{
    const int value = esp_mcp_property_list_get_property_int(properties, "volume");
    if (value < settings_storage::VOLUME_MIN || value > settings_storage::VOLUME_MAX ||
            board_codec_volume_set(value, nullptr) != ESP_OK) {
        return esp_mcp_value_create_bool(false);
    }
    settings_storage::saveVolume(value);
    return esp_mcp_value_create_bool(true);
}

esp_mcp_value_t setBrightness(const esp_mcp_property_list_t *properties)
{
    const int value = esp_mcp_property_list_get_property_int(properties, "brightness");
    if (value < settings_storage::BRIGHTNESS_MIN || value > settings_storage::BRIGHTNESS_MAX ||
            board_display_brightness_set(value) != ESP_OK) {
        return esp_mcp_value_create_bool(false);
    }
    settings_storage::saveBrightness(value);
    return esp_mcp_value_create_bool(true);
}

esp_mcp_value_t setMute(const esp_mcp_property_list_t *properties)
{
    const bool mute = esp_mcp_property_list_get_property_bool(properties, "mute");
    return esp_mcp_value_create_bool(board_codec_mute_set(mute) == ESP_OK);
}

// Turning the screen back on restores the saved brightness instead of the board's 100 %.
esp_mcp_value_t setScreen(const esp_mcp_property_list_t *properties)
{
    const bool on = esp_mcp_property_list_get_property_bool(properties, "on");
    const esp_err_t ret = on ? board_display_brightness_set(settings_storage::loadBrightness()) :
                          board_display_backlight_off();
    return esp_mcp_value_create_bool(ret == ESP_OK);
}

// Same tool name and arguments as upstream xiaozhi-esp32; applies the next time the Xiaozhi page opens.
// TODO(theme): registered only when XIAOZHI_THEME_ENABLED.
[[maybe_unused]] esp_mcp_value_t setTheme(const esp_mcp_property_list_t *properties)
{
    const char *theme = esp_mcp_property_list_get_property_string(properties, "theme");
    ESP_LOGI("XiaozhiTools", "self.screen.set_theme(%s)", theme ? theme : "null");
    if (theme && std::strcmp(theme, "dark") == 0) {
        settings_storage::saveTheme(settings_storage::THEME_DARK);
        return esp_mcp_value_create_bool(true);
    }
    if (theme && std::strcmp(theme, "light") == 0) {
        settings_storage::saveTheme(settings_storage::THEME_LIGHT);
        return esp_mcp_value_create_bool(true);
    }
    return esp_mcp_value_create_bool(false);
}

// The three tools below use the names and JSON fields of the upstream firmware: the console and
// the assets generator call them to read the chip, the screen size and the "assets" partition.
esp_mcp_value_t getScreenInfo(const esp_mcp_property_list_t *)
{
    board_display_info_t display = {};
    if (board_display_get_info(&display) != ESP_OK) {
        return esp_mcp_value_create_string("{}");
    }
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");
    cJSON_AddNumberToObject(root, "width", display.width);
    cJSON_AddNumberToObject(root, "height", display.height);
    cJSON_AddBoolToObject(root, "monochrome", false);
    return jsonValue(root);
}

esp_mcp_value_t getSystemInfo(const esp_mcp_property_list_t *)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");

    uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) == ESP_OK) {
        cJSON_AddNumberToObject(root, "flash_size", flash_size);
    }
    cJSON_AddNumberToObject(root, "minimum_free_heap_size", esp_get_minimum_free_heap_size());
    cJSON_AddStringToObject(root, "chip_model_name", CONFIG_IDF_TARGET);

    const esp_app_desc_t *app = esp_app_get_description();
    cJSON *application = cJSON_AddObjectToObject(root, "application");
    cJSON_AddStringToObject(application, "name", app->project_name);
    cJSON_AddStringToObject(application, "version", app->version);
    cJSON_AddStringToObject(application, "compile_time", app->date);
    cJSON_AddStringToObject(application, "idf_version", esp_get_idf_version());

    cJSON *partitions = cJSON_AddArrayToObject(root, "partition_table");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    for (; it; it = esp_partition_next(it)) {
        const esp_partition_t *partition = esp_partition_get(it);
        cJSON *item = cJSON_CreateObject();
        if (!item) break;
        cJSON_AddStringToObject(item, "label", partition->label);
        cJSON_AddNumberToObject(item, "type", partition->type);
        cJSON_AddNumberToObject(item, "subtype", partition->subtype);
        cJSON_AddNumberToObject(item, "address", partition->address);
        cJSON_AddNumberToObject(item, "size", partition->size);
        cJSON_AddItemToArray(partitions, item);
    }
    esp_partition_iterator_release(it);

    cJSON *board = cJSON_AddObjectToObject(root, "board");
    cJSON_AddStringToObject(board, "name", "esp32-p4-wifi6-db-touch-lcd-7c");
    return jsonValue(root);
}

esp_mcp_value_t getDeviceStatus(const esp_mcp_property_list_t *)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");

    cJSON *speaker = cJSON_AddObjectToObject(root, "audio_speaker");
    cJSON_AddNumberToObject(speaker, "volume", board_codec_volume_get());
    cJSON *screen = cJSON_AddObjectToObject(root, "screen");
    cJSON_AddNumberToObject(screen, "brightness", board_display_brightness_get());
    cJSON_AddStringToObject(screen, "theme",
                            settings_storage::loadTheme() == settings_storage::THEME_LIGHT ? "light" : "dark");

    cJSON *network = cJSON_AddObjectToObject(root, "network");
    cJSON_AddStringToObject(network, "type", "wifi");
    wifi_ap_record_t ap = {};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        cJSON_AddStringToObject(network, "ssid", reinterpret_cast<const char *>(ap.ssid));
        const char *signal = ap.rssi >= -60 ? "strong" : ap.rssi >= -70 ? "medium" : ap.rssi >= -80 ? "weak" : "very weak";
        cJSON_AddStringToObject(network, "signal", signal);
    }
    return jsonValue(root);
}

esp_mcp_value_t getStatus(const esp_mcp_property_list_t *)
{
    char status[192];
    std::snprintf(status, sizeof(status),
                  "{\"volume\":%d,\"brightness\":%d,\"internal_free_bytes\":%lu,\"psram_free_bytes\":%lu}",
                  board_codec_volume_get(), board_display_brightness_get(),
                  static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                  static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
    return esp_mcp_value_create_string(status);
}

esp_mcp_value_t getWifiStatus(const esp_mcp_property_list_t *)
{
    static constexpr const char *state_names[] = {"idle", "connecting", "associated", "connected"};
    board_wifi_link_t link = {};
    board_wifi_service_get_link(&link);
    const size_t state = static_cast<size_t>(link.state);

    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");
    cJSON_AddBoolToObject(root, "enabled", board_wifi_service_is_enabled());
    cJSON_AddStringToObject(root, "state", state < sizeof(state_names) / sizeof(state_names[0]) ?
                            state_names[state] : "unknown");
    cJSON_AddStringToObject(root, "ssid", link.ssid);

    esp_netif_ip_info_t ip_info = {};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        char ip[16];
        std::snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_info.ip));
        cJSON_AddStringToObject(root, "ip", ip);
    }
    return jsonValue(root);
}

esp_mcp_value_t getDeviceInfo(const esp_mcp_property_list_t *)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");

    esp_chip_info_t chip = {};
    esp_chip_info(&chip);
    char chip_text[32];
    std::snprintf(chip_text, sizeof(chip_text), "%s rev%d.%d", CONFIG_IDF_TARGET,
                  chip.revision / 100, chip.revision % 100);
    cJSON_AddStringToObject(root, "chip", chip_text);
    cJSON_AddStringToObject(root, "firmware_version", esp_app_get_description()->version);
    cJSON_AddStringToObject(root, "idf_version", esp_get_idf_version());

    board_display_info_t display = {};
    if (board_display_get_info(&display) == ESP_OK) {
        cJSON *item = cJSON_AddObjectToObject(root, "display");
        cJSON_AddNumberToObject(item, "width", display.width);
        cJSON_AddNumberToObject(item, "height", display.height);
    }

    board_mic_layout_t mics = {};
    if (board_audio_get_mic_layout(&mics) == ESP_OK) {
        cJSON *item = cJSON_AddObjectToObject(root, "microphones");
        cJSON_AddNumberToObject(item, "count", mics.mic_num);
        cJSON_AddBoolToObject(item, "echo_reference", mics.echo_ref_channel_index >= 0);
    }

    board_wifi_coprocessor_info_t coprocessor = {};
    if (board_wifi_get_coprocessor_info(&coprocessor) == ESP_OK) {
        cJSON *item = cJSON_AddObjectToObject(root, "wifi_coprocessor");
        cJSON_AddStringToObject(item, "chip", coprocessor.chip ? coprocessor.chip : "unknown");
        if (coprocessor.valid) {
            char version[24];
            std::snprintf(version, sizeof(version), "%lu.%lu.%lu",
                          static_cast<unsigned long>(coprocessor.major),
                          static_cast<unsigned long>(coprocessor.minor),
                          static_cast<unsigned long>(coprocessor.patch));
            cJSON_AddStringToObject(item, "firmware_version", version);
        }
    }
    return jsonValue(root);
}

esp_mcp_value_t setUserLed(const esp_mcp_property_list_t *properties)
{
    const int led = esp_mcp_property_list_get_property_int(properties, "led");
    const bool on = esp_mcp_property_list_get_property_bool(properties, "on");
    // The tool numbers the LEDs from 1, like the silkscreen.
    return esp_mcp_value_create_bool(board_user_led_set(led - 1, on) == ESP_OK);
}

esp_mcp_value_t getUserLeds(const esp_mcp_property_list_t *)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string("{}");
    for (int i = 0; i < BOARD_USER_LED_COUNT; ++i) {
        bool on = false;
        (void)board_user_led_get(i, &on);
        char key[8];
        std::snprintf(key, sizeof(key), "led%d", i + 1);
        cJSON_AddBoolToObject(root, key, on);
    }
    return jsonValue(root);
}

esp_mcp_value_t getRtcTime(const esp_mcp_property_list_t *)
{
    board_rtc_time_t time = {};
    if (board_rtc_get_time(&time) != ESP_OK) {
        return esp_mcp_value_create_string("RTC unavailable");
    }
    // The fields are range-checked on write, but the compiler sizes the buffer for full-width values.
    char text[48];
    std::snprintf(text, sizeof(text), "%04u-%02u-%02u %02u:%02u:%02u", static_cast<unsigned>(time.year),
                  static_cast<unsigned>(time.month), static_cast<unsigned>(time.day),
                  static_cast<unsigned>(time.hour), static_cast<unsigned>(time.minute),
                  static_cast<unsigned>(time.second));
    cJSON *root = cJSON_CreateObject();
    if (!root) return esp_mcp_value_create_string(text);
    cJSON_AddStringToObject(root, "time", text);
    cJSON_AddNumberToObject(root, "weekday", time.weekday);
    return jsonValue(root);
}

// Argument is "YYYY-MM-DD HH:MM:SS"; the weekday is derived from the date.
esp_mcp_value_t setRtcTime(const esp_mcp_property_list_t *properties)
{
    const char *text = esp_mcp_property_list_get_property_string(properties, "datetime");
    unsigned year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (!text || std::sscanf(text, "%u-%u-%u %u:%u:%u", &year, &month, &day, &hour, &minute, &second) != 6) {
        return esp_mcp_value_create_bool(false);
    }
    static constexpr int month_offset[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 1 || month > 12) {
        return esp_mcp_value_create_bool(false);
    }
    const int y = static_cast<int>(year) - (month < 3 ? 1 : 0);
    const int weekday = (y + y / 4 - y / 100 + y / 400 + month_offset[month - 1] + static_cast<int>(day)) % 7;
    board_rtc_time_t time = {};
    time.year = static_cast<uint16_t>(year);
    time.month = static_cast<uint8_t>(month);
    time.day = static_cast<uint8_t>(day);
    time.weekday = static_cast<uint8_t>(weekday);
    time.hour = static_cast<uint8_t>(hour);
    time.minute = static_cast<uint8_t>(minute);
    time.second = static_cast<uint8_t>(second);
    return esp_mcp_value_create_bool(board_rtc_set_time(&time) == ESP_OK);
}

void rebootTimerCallback(void *)
{
    esp_restart();
}

esp_mcp_value_t reboot(const esp_mcp_property_list_t *)
{
    std::lock_guard<std::mutex> lock(reboot_mutex);
    if (!reboot_timer) {
        esp_timer_create_args_t args = {};
        args.callback = rebootTimerCallback;
        args.name = "xiaozhi_reboot";
        if (esp_timer_create(&args, &reboot_timer) != ESP_OK) {
            reboot_timer = nullptr;
            return esp_mcp_value_create_bool(false);
        }
    }
    // A second request while the timer is pending keeps the first schedule.
    const esp_err_t ret = esp_timer_start_once(reboot_timer, REBOOT_DELAY_US);
    return esp_mcp_value_create_bool(ret == ESP_OK || ret == ESP_ERR_INVALID_STATE);
}

// Only stores the URL, as upstream does: the console follows up with self.reboot, and the package is
// downloaded after the restart, when the network is up and nothing has loaded it.
#if XIAOZHI_THEME_ENABLED  // TODO(theme)
esp_mcp_value_t setAssetsUrl(const esp_mcp_property_list_t *properties)
{
    const char *url = esp_mcp_property_list_get_property_string(properties, "url");
    return esp_mcp_value_create_bool(xiaozhi_assets::setDownloadUrl(url));
}
#endif

esp_err_t addTool(esp_mcp_t *mcp, const char *name, const char *description,
                  esp_mcp_tool_callback_t callback)
{
    esp_mcp_tool_t *tool = esp_mcp_tool_create(name, description, callback);
    if (!tool) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = esp_mcp_add_tool(mcp, tool);
    if (ret != ESP_OK) {
        esp_mcp_tool_destroy(tool);
    }
    return ret;
}

// Takes ownership of property, which may be NULL when its allocation failed.
esp_err_t addTool(esp_mcp_t *mcp, const char *name, const char *description,
                  esp_mcp_tool_callback_t callback, esp_mcp_property_t *property)
{
    if (!property) {
        return ESP_ERR_NO_MEM;
    }
    esp_mcp_tool_t *tool = esp_mcp_tool_create(name, description, callback);
    if (!tool) {
        esp_mcp_property_destroy(property);
        return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = esp_mcp_tool_add_property(tool, property);
    if (ret != ESP_OK) {
        esp_mcp_property_destroy(property);
        esp_mcp_tool_destroy(tool);
        return ret;
    }
    ret = esp_mcp_add_tool(mcp, tool);
    if (ret != ESP_OK) {
        esp_mcp_tool_destroy(tool);
    }
    return ret;
}

} // namespace

void setXiaozhiDeveloperToolsChat(esp_xiaozhi_chat_handle_t chat)
{
    std::lock_guard<std::mutex> lock(camera_mutex);
    camera_chat = chat;
}

esp_err_t registerXiaozhiDeveloperTools(esp_mcp_t *mcp)
{
    if (!mcp) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = addTool(mcp, "device.audio.set_volume", "Set speaker volume from 0 to 95 and save it.", setVolume,
                            esp_mcp_property_create_with_range("volume", settings_storage::VOLUME_MIN,
                                                               settings_storage::VOLUME_MAX));
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.audio.set_mute", "Mute (true) or unmute (false) the speaker.", setMute,
                  esp_mcp_property_create("mute", ESP_MCP_PROPERTY_TYPE_BOOLEAN));
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.display.set_brightness", "Set display brightness from 20 to 100 and save it.",setBrightness,
                  esp_mcp_property_create_with_range("brightness", settings_storage::BRIGHTNESS_MIN,
                                                     settings_storage::BRIGHTNESS_MAX));
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.display.set_screen","Turn the screen backlight on (true) or off (false). Turning it on restores the saved brightness.",setScreen, esp_mcp_property_create("on", ESP_MCP_PROPERTY_TYPE_BOOLEAN));
    if (ret != ESP_OK) return ret;
#if XIAOZHI_THEME_ENABLED  // TODO(theme)
    ret = addTool(mcp, "self.screen.set_theme", "Set the theme of the screen. The theme can be `light` or `dark`.", setTheme,
                  esp_mcp_property_create("theme", ESP_MCP_PROPERTY_TYPE_STRING));
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "self.assets.set_download_url", "Set the download url for the assets", setAssetsUrl,
                  esp_mcp_property_create("url", ESP_MCP_PROPERTY_TYPE_STRING));
    if (ret != ESP_OK) return ret;
#endif
    ret = addTool(mcp, "self.screen.get_info", "Information about the screen, including width, height, etc.", getScreenInfo);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "self.get_system_info", "Get the system information: chip, firmware, flash size and partition table.", getSystemInfo);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "self.get_device_status", "Provides the real-time status of the device: speaker volume, screen and network.", getDeviceStatus);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.get_status", "Get volume, display brightness and free memory in bytes.", getStatus);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.network.get_wifi_status","Get whether Wi-Fi is enabled, the link state, the SSID and the IPv4 address.", getWifiStatus);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.get_info","Get the chip, firmware and ESP-IDF versions, display size, microphones and Wi-Fi co-processor.",getDeviceInfo);
    if (ret != ESP_OK) return ret;
    {
        esp_mcp_tool_t *led_tool = esp_mcp_tool_create("device.led.set", "Turn user LED 1 or 2 on (true) or off (false).", setUserLed);
        if (!led_tool) return ESP_ERR_NO_MEM;
        esp_mcp_property_t *led_index = esp_mcp_property_create_with_range("led", 1, BOARD_USER_LED_COUNT);
        esp_mcp_property_t *led_on = esp_mcp_property_create("on", ESP_MCP_PROPERTY_TYPE_BOOLEAN);
        if (!led_index || !led_on) {
            if (led_index) esp_mcp_property_destroy(led_index);
            if (led_on) esp_mcp_property_destroy(led_on);
            esp_mcp_tool_destroy(led_tool);
            return ESP_ERR_NO_MEM;
        }
        ret = esp_mcp_tool_add_property(led_tool, led_index);
        if (ret == ESP_OK) {
            ret = esp_mcp_tool_add_property(led_tool, led_on);
        } else {
            esp_mcp_property_destroy(led_on);
        }
        if (ret == ESP_OK) ret = esp_mcp_add_tool(mcp, led_tool);
        if (ret != ESP_OK) {
            esp_mcp_tool_destroy(led_tool);
            return ret;
        }
    }
    ret = addTool(mcp, "device.led.get", "Get the on/off state of the user LEDs.", getUserLeds);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.rtc.get_time", "Get the date and time kept by the on-board RTC.", getRtcTime);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.rtc.set_time", "Set the on-board RTC. datetime is formatted as YYYY-MM-DD HH:MM:SS.", setRtcTime,
                  esp_mcp_property_create("datetime", ESP_MCP_PROPERTY_TYPE_STRING));
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "self.reboot", "Reboot the system", reboot);
    if (ret != ESP_OK) return ret;
    ret = addTool(mcp, "device.system.reboot","Restart the device after a few seconds. Only call this when the user explicitly asks to restart.",reboot);
    if (ret != ESP_OK) return ret;
    esp_mcp_tool_t *tool = esp_mcp_tool_create("self.camera.explain_image", "Take a photo and answer a question about it.", explainImage);
    if (!tool) return ESP_ERR_NO_MEM;
    ret = esp_mcp_tool_set_task_support(tool, "forbidden");
    if (ret != ESP_OK) {
        esp_mcp_tool_destroy(tool);
        return ret;
    }
    esp_mcp_property_t *question = esp_mcp_property_create("question", ESP_MCP_PROPERTY_TYPE_STRING);
    if (!question) {
        esp_mcp_tool_destroy(tool);
        return ESP_ERR_NO_MEM;
    }
    ret = esp_mcp_tool_add_property(tool, question);
    if (ret != ESP_OK) {
        esp_mcp_property_destroy(question);
        esp_mcp_tool_destroy(tool);
        return ret;
    }
    ret = esp_mcp_add_tool(mcp, tool);
    if (ret != ESP_OK) esp_mcp_tool_destroy(tool);
    return ret;
}

} // namespace esp_brookesia::apps
