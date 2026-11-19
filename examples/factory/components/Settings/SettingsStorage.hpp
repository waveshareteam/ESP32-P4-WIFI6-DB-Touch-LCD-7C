#pragma once

#include "board_audio.h"

/*
 * Persisted user settings, stored through the Brookesia StorageNVS service.
 * The service must have been started (StorageNVS::begin()) before load*() is
 * called; until then, and when the service is disabled, the defaults are used.
 */
namespace esp_brookesia::apps::settings_storage {

constexpr int BRIGHTNESS_MIN = 20;
constexpr int BRIGHTNESS_MAX = 100;
constexpr int BRIGHTNESS_DEFAULT = 100;

constexpr int VOLUME_MIN = 0;
constexpr int VOLUME_MAX = 95;
constexpr int VOLUME_DEFAULT = CODEC_DEFAULT_VOLUME;

/** Saved brightness in percent, clamped to [BRIGHTNESS_MIN, BRIGHTNESS_MAX]. */
int loadBrightness();
void saveBrightness(int percent);

/** Xiaozhi page theme: 0 = dark (the default, matching the system UI and the console's dark skin), 1 = light. */
constexpr int THEME_DARK = 0;
constexpr int THEME_LIGHT = 1;
// TODO(theme): was THEME_DARK.
constexpr int THEME_DEFAULT = THEME_LIGHT;

int loadTheme();
void saveTheme(int theme);

/** Saved media volume in percent, clamped to [VOLUME_MIN, VOLUME_MAX]. */
int loadVolume();
void saveVolume(int percent);

} // namespace esp_brookesia::apps::settings_storage
