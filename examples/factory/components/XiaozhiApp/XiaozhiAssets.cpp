/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XiaozhiAssets.hpp"

#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <map>
#include <string>

#include "cJSON.h"
#include "cbin_font.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

namespace esp_brookesia::apps::xiaozhi_assets {

namespace {

constexpr char TAG[] = "XiaozhiAssets";
constexpr char PARTITION_LABEL[] = "assets";
constexpr char NVS_NAMESPACE[] = "assets";
constexpr char NVS_KEY_URL[] = "download_url";
constexpr size_t MAX_URL_LENGTH = 512;
constexpr size_t HEADER_SIZE = 12;
constexpr size_t CHUNK_SIZE = 4096;
constexpr int DOWNLOAD_TIMEOUT_MS = 20000;
constexpr uint32_t DOWNLOAD_TASK_STACK = 8 * 1024;

// One entry of the file table that follows the 12-byte header; the layout is fixed by the packer.
struct TableEntry {
    char name[32];
    uint32_t size;
    uint32_t offset;
    uint16_t width;
    uint16_t height;
};

struct Asset {
    size_t offset;
    size_t size;
};

std::atomic<bool> s_downloading{false};
std::atomic<bool> s_download_started{false};
std::atomic<bool> s_loading{false};
bool s_loaded = false;
const uint8_t *s_root = nullptr; // PSRAM copy of the package; lives for the rest of the run
std::map<std::string, Asset> s_assets;
Package s_package;
bool s_package_valid = false;

uint32_t checksumBytes(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum += data[i];
    }
    return sum;
}

// Returns a pointer past the "ZZ" magic that precedes every asset.
const uint8_t *assetData(const char *name, size_t &size)
{
    auto it = s_assets.find(name);
    if (it == s_assets.end()) {
        return nullptr;
    }
    const uint8_t *data = s_root + it->second.offset;
    if (data[0] != 'Z' || data[1] != 'Z') {
        ESP_LOGE(TAG, "Asset %s has no valid magic", name);
        return nullptr;
    }
    size = it->second.size;
    return data + 2;
}

bool parseColor(const char *text, uint32_t &out)
{
    if (!text) {
        return false;
    }
    if (text[0] == '#') {
        ++text;
    }
    char *end = nullptr;
    const unsigned long value = strtoul(text, &end, 16);
    if (end == text || *end != '\0' || value > 0xFFFFFF) {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

void parseSkin(cJSON *skin, Skin &out)
{
    if (!cJSON_IsObject(skin)) {
        return;
    }
    cJSON *text_color = cJSON_GetObjectItem(skin, "text_color");
    if (cJSON_IsString(text_color)) {
        out.has_text_color = parseColor(text_color->valuestring, out.text_color);
    }
    cJSON *background_color = cJSON_GetObjectItem(skin, "background_color");
    if (cJSON_IsString(background_color)) {
        out.has_background_color = parseColor(background_color->valuestring, out.background_color);
    }
    cJSON *background_image = cJSON_GetObjectItem(skin, "background_image");
    if (cJSON_IsString(background_image)) {
        size_t size = 0;
        const uint8_t *data = assetData(background_image->valuestring, size);
        if (data) {
            out.background_image = cbin_img_dsc_create(const_cast<uint8_t *>(data));
        } else {
            ESP_LOGW(TAG, "Background image %s is missing", background_image->valuestring);
        }
    }
}

bool mapAndParse()
{
    const esp_partition_t *partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, PARTITION_LABEL);
    if (!partition) {
        ESP_LOGI(TAG, "No assets partition");
        return false;
    }

    uint8_t header[HEADER_SIZE] = {};
    if (esp_partition_read(partition, 0, header, sizeof(header)) != ESP_OK) {
        ESP_LOGW(TAG, "Read assets header failed");
        return false;
    }
    uint32_t files = 0, stored_checksum = 0, stored_length = 0;
    memcpy(&files, header, 4);
    memcpy(&stored_checksum, header + 4, 4);
    memcpy(&stored_length, header + 8, 4);
    if (files == 0 || files > 4096 || stored_length == 0 || stored_length > partition->size - HEADER_SIZE ||
            stored_length < files * sizeof(TableEntry)) {
        ESP_LOGI(TAG, "No valid assets package in the partition");
        return false;
    }

    // Quad flash above 16 MiB cannot be memory-mapped (24-bit cache addresses), so the package is read
    // into PSRAM instead. The partition API itself handles the 32-bit addresses.
    const size_t map_size = HEADER_SIZE + stored_length;
    auto *copy = static_cast<uint8_t *>(heap_caps_malloc(map_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!copy) {
        ESP_LOGE(TAG, "No PSRAM for the assets package (%u bytes)", static_cast<unsigned>(map_size));
        return false;
    }
    for (size_t offset = 0; offset < map_size; offset += CHUNK_SIZE * 16) {
        const size_t length = map_size - offset < CHUNK_SIZE * 16 ? map_size - offset : CHUNK_SIZE * 16;
        if (esp_partition_read(partition, offset, copy + offset, length) != ESP_OK) {
            ESP_LOGE(TAG, "Read assets package failed at offset %u", static_cast<unsigned>(offset));
            heap_caps_free(copy);
            return false;
        }
    }
    s_root = copy;

    const uint32_t checksum = checksumBytes(s_root + HEADER_SIZE, stored_length) & 0xFFFF;
    if (checksum != stored_checksum) {
        ESP_LOGE(TAG, "Assets checksum mismatch (0x%lx != 0x%lx)", static_cast<unsigned long>(checksum),
                 static_cast<unsigned long>(stored_checksum));
        heap_caps_free(copy);
        s_root = nullptr;
        return false;
    }

    s_assets.clear();
    const size_t table_size = files * sizeof(TableEntry);
    for (uint32_t i = 0; i < files; ++i) {
        TableEntry entry;
        memcpy(&entry, s_root + HEADER_SIZE + i * sizeof(TableEntry), sizeof(entry));
        entry.name[sizeof(entry.name) - 1] = '\0';
        const size_t offset = HEADER_SIZE + table_size + entry.offset;
        if (offset + entry.size > map_size) {
            ESP_LOGW(TAG, "Asset %s lies outside the package", entry.name);
            continue;
        }
        s_assets[entry.name] = {offset, entry.size};
    }

    size_t index_size = 0;
    const uint8_t *index_data = assetData("index.json", index_size);
    if (!index_data) {
        ESP_LOGE(TAG, "index.json is missing");
        return false;
    }
    cJSON *root = cJSON_ParseWithLength(reinterpret_cast<const char *>(index_data), index_size);
    if (!root) {
        ESP_LOGE(TAG, "index.json is not valid");
        return false;
    }

    cJSON *version = cJSON_GetObjectItem(root, "version");
    if (cJSON_IsNumber(version) && version->valuedouble > 1) {
        ESP_LOGE(TAG, "Assets version %d is not supported", version->valueint);
        cJSON_Delete(root);
        return false;
    }

    cJSON *font = cJSON_GetObjectItem(root, "text_font");
    if (cJSON_IsString(font)) {
        size_t size = 0;
        const uint8_t *data = assetData(font->valuestring, size);
        if (data) {
            s_package.text_font = cbin_font_create(const_cast<uint8_t *>(data));
        }
        if (!s_package.text_font) {
            ESP_LOGW(TAG, "Text font %s is missing or invalid", font->valuestring);
        }
    }
    cJSON *emoji_collection = cJSON_GetObjectItem(root, "emoji_collection");
    if (cJSON_IsArray(emoji_collection)) {
        const int count = cJSON_GetArraySize(emoji_collection);
        for (int i = 0; i < count; ++i) {
            cJSON *item = cJSON_GetArrayItem(emoji_collection, i);
            cJSON *name = cJSON_GetObjectItem(item, "name");
            cJSON *file = cJSON_GetObjectItem(item, "file");
            // Entries with an "eaf" field are animation packs for other display types.
            if (!cJSON_IsString(name) || !cJSON_IsString(file) || cJSON_GetObjectItem(item, "eaf")) {
                continue;
            }
            size_t size = 0;
            const uint8_t *data = assetData(file->valuestring, size);
            if (!data || size < 4) {
                ESP_LOGW(TAG, "Emoji %s image %s is missing", name->valuestring, file->valuestring);
                continue;
            }
            EmojiImage &image = s_package.emoji[name->valuestring];
            image.dsc = {};
            image.dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            image.dsc.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
            image.dsc.data_size = size;
            image.dsc.data = data;
            image.is_gif = data[0] == 'G' && data[1] == 'I' && data[2] == 'F';
        }
    }
    cJSON *skin = cJSON_GetObjectItem(root, "skin");
    if (cJSON_IsObject(skin)) {
        parseSkin(cJSON_GetObjectItem(skin, "light"), s_package.light);
        parseSkin(cJSON_GetObjectItem(skin, "dark"), s_package.dark);
    }
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Assets package loaded: %u files, font=%s, emoji=%u", static_cast<unsigned>(files),
             s_package.text_font ? "yes" : "no", static_cast<unsigned>(s_package.emoji.size()));
    return true;
}

esp_err_t downloadTo(const esp_partition_t *partition, const char *url)
{
    esp_http_client_config_t config = {};
    config.url = url;
    config.timeout_ms = DOWNLOAD_TIMEOUT_MS;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.buffer_size = 2048;
    config.keep_alive_enable = true;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = esp_http_client_open(client, 0);
    if (ret != ESP_OK) {
        esp_http_client_cleanup(client);
        return ret;
    }
    const int64_t length = esp_http_client_fetch_headers(client);
    const int status = esp_http_client_get_status_code(client);
    if (status != 200 || length < static_cast<int64_t>(HEADER_SIZE) || length > static_cast<int64_t>(partition->size)) {
        ESP_LOGE(TAG, "Download rejected: status=%d length=%lld (partition %lu)", status, static_cast<long long>(length),
                 static_cast<unsigned long>(partition->size));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_ERR_INVALID_SIZE;
    }

    const size_t sector = esp_partition_get_main_flash_sector_size();
    auto *buffer = static_cast<uint8_t *>(heap_caps_malloc(CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!buffer) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }

    uint8_t header[HEADER_SIZE] = {};
    size_t header_have = 0;
    size_t payload_written = 0;
    size_t erased_sectors = 0;
    uint32_t checksum = 0;
    int last_percent = -1;
    ret = ESP_OK;

    while (true) {
        const int n = esp_http_client_read(client, reinterpret_cast<char *>(buffer), CHUNK_SIZE);
        if (n < 0) {
            ESP_LOGE(TAG, "Download read failed");
            ret = ESP_FAIL;
            break;
        }
        if (n == 0) {
            if (esp_http_client_is_complete_data_received(client)) {
                break;
            }
            ESP_LOGE(TAG, "Download ended early");
            ret = ESP_ERR_INVALID_SIZE;
            break;
        }

        size_t pos = 0;
        if (header_have < HEADER_SIZE) {
            const size_t take = HEADER_SIZE - header_have < static_cast<size_t>(n) ? HEADER_SIZE - header_have
                                                                                    : static_cast<size_t>(n);
            memcpy(header + header_have, buffer, take);
            header_have += take;
            pos = take;
        }
        if (static_cast<size_t>(n) > pos) {
            const size_t write_len = static_cast<size_t>(n) - pos;
            const size_t end = HEADER_SIZE + payload_written + write_len;
            const size_t needed_sectors = (end + sector - 1) / sector;
            while (erased_sectors < needed_sectors) {
                ret = esp_partition_erase_range(partition, erased_sectors * sector, sector);
                if (ret != ESP_OK) {
                    break;
                }
                ++erased_sectors;
            }
            if (ret != ESP_OK) {
                break;
            }
            ret = esp_partition_write(partition, HEADER_SIZE + payload_written, buffer + pos, write_len);
            if (ret != ESP_OK) {
                break;
            }
            checksum += checksumBytes(buffer + pos, write_len);
            payload_written += write_len;
        }

        const int percent = static_cast<int>((header_have + payload_written) * 100 / static_cast<size_t>(length));
        if (percent / 10 != last_percent / 10) {
            ESP_LOGI(TAG, "Download %d%%", percent);
            last_percent = percent;
        }
    }
    heap_caps_free(buffer);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (ret == ESP_OK && header_have + payload_written != static_cast<size_t>(length)) {
        ESP_LOGE(TAG, "Downloaded %u of %lld bytes", static_cast<unsigned>(header_have + payload_written),
                 static_cast<long long>(length));
        ret = ESP_ERR_INVALID_SIZE;
    }
    if (ret == ESP_OK) {
        uint32_t stored_checksum = 0, stored_length = 0;
        memcpy(&stored_checksum, header + 4, 4);
        memcpy(&stored_length, header + 8, 4);
        if (stored_length != payload_written || (checksum & 0xFFFF) != stored_checksum) {
            ESP_LOGE(TAG, "Package verification failed (length %lu/%u, checksum 0x%lx/0x%lx)",
                     static_cast<unsigned long>(stored_length), static_cast<unsigned>(payload_written),
                     static_cast<unsigned long>(checksum & 0xFFFF), static_cast<unsigned long>(stored_checksum));
            ret = ESP_ERR_INVALID_CRC;
        }
    }
    // The header goes in last, so an interrupted or corrupt download never looks like a package.
    if (ret == ESP_OK) {
        ret = esp_partition_write(partition, 0, header, sizeof(header));
    }
    return ret;
}

void downloadTask(void *)
{
    char url[MAX_URL_LENGTH] = {};
    nvs_handle_t nvs = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        size_t size = sizeof(url);
        if (nvs_get_str(nvs, NVS_KEY_URL, url, &size) != ESP_OK) {
            url[0] = '\0';
        }
        // Forget the URL first so a bad package cannot cause a download on every boot.
        (void)nvs_erase_key(nvs, NVS_KEY_URL);
        (void)nvs_commit(nvs);
        nvs_close(nvs);
    }

    const esp_partition_t *partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, PARTITION_LABEL);
    if (url[0] && partition) {
        ESP_LOGI(TAG, "Downloading assets package");
        const esp_err_t ret = downloadTo(partition, url);
        ESP_LOGI(TAG, "Assets download %s (%s)", ret == ESP_OK ? "finished" : "failed", esp_err_to_name(ret));
    } else if (url[0]) {
        ESP_LOGW(TAG, "Assets URL is set but there is no assets partition");
    }
    s_downloading.store(false);
    vTaskDelete(nullptr);
}

} // namespace

const Package *load()
{
    if (s_downloading.load()) {
        return nullptr;
    }
    if (!s_loaded) {
        s_loaded = true;
        s_loading.store(true);
        s_package_valid = mapAndParse();
        s_loading.store(false);
    }
    return s_package_valid ? &s_package : nullptr;
}

bool setDownloadUrl(const char *url)
{
    if (!url || strlen(url) == 0 || strlen(url) >= MAX_URL_LENGTH ||
            (strncmp(url, "https://", 8) != 0 && strncmp(url, "http://", 7) != 0)) {
        return false;
    }
    nvs_handle_t nvs = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return false;
    }
    bool ok = nvs_set_str(nvs, NVS_KEY_URL, url) == ESP_OK && nvs_commit(nvs) == ESP_OK;
    nvs_close(nvs);
    return ok;
}

void startPendingDownload()
{
    // The loaded package lives in a PSRAM copy, so rewriting the partition never disturbs fonts and
    // images in use; a new package takes effect on the next boot. Only skip while the partition is
    // being read, and download at most once per boot.
    if (s_loading.load() || s_download_started.exchange(true)) {
        return;
    }

    nvs_handle_t nvs = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }
    size_t size = 0;
    const bool pending = nvs_get_str(nvs, NVS_KEY_URL, nullptr, &size) == ESP_OK && size > 1;
    nvs_close(nvs);
    if (!pending) {
        return;
    }

    s_downloading.store(true);
    if (xTaskCreate(downloadTask, "assets_dl", DOWNLOAD_TASK_STACK, nullptr, 5, nullptr) != pdPASS) {
        s_downloading.store(false);
        ESP_LOGE(TAG, "Create download task failed");
    }
}

} // namespace esp_brookesia::apps::xiaozhi_assets
