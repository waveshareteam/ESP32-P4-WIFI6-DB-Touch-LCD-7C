/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include <map>
#include <string>

#include "lvgl.h"

/*
 * Custom theme package ("assets.bin") pushed by the Xiaozhi console.
 *
 * Flow, the same as the upstream xiaozhi-esp32 firmware:
 *   1. The server calls the MCP tool self.assets.set_download_url; the URL is kept in NVS and the
 *      device restarts.
 *   2. Once the network is up after the restart, the package is downloaded into the "assets"
 *      partition. The package in use is a PSRAM copy, so this never disturbs the running UI.
 *   3. The Xiaozhi page reads the package (copied into PSRAM once per boot) when it is created: text font, skin colours and the
 *      chat background image. Custom emoji (PNG/GIF) are shown in place of the built-in emoji font; wake-word models in the package are ignored.
 */
namespace esp_brookesia::apps::xiaozhi_assets {

struct Skin {
    bool has_text_color = false;
    uint32_t text_color = 0;
    bool has_background_color = false;
    uint32_t background_color = 0;
    const lv_image_dsc_t *background_image = nullptr;
};

// A raw PNG or GIF kept in the mapped package; the LVGL decoders read it in place.
struct EmojiImage {
    lv_image_dsc_t dsc = {};
    bool is_gif = false;
};

struct Package {
    const lv_font_t *text_font = nullptr;
    Skin light;
    Skin dark;
    std::map<std::string, EmojiImage> emoji;

    const EmojiImage *findEmoji(const char *name) const
    {
        auto it = emoji.find(name ? name : "");
        return it == emoji.end() ? nullptr : &it->second;
    }
};

/**
 * Map and parse the package once and return it, or nullptr when there is no valid package or a
 * download is running. The caller must hold the LVGL lock: fonts and images are created here.
 */
const Package *load();

/** Remember a download URL for the next boot. Returns false for an invalid URL. */
bool setDownloadUrl(const char *url);

/** Start the download task if a URL is pending. Safe to call repeatedly; call once the network is up. */
void startPendingDownload();

} // namespace esp_brookesia::apps::xiaozhi_assets
