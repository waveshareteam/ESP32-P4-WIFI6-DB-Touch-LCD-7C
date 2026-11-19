/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XiaozhiUi.hpp"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "cbin_font.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "font_awesome.h"
#include "font_emoji.h"
#include "SettingsStorage.hpp"
#if XIAOZHI_THEME_ENABLED
#include "XiaozhiAssets.hpp"
#endif

LV_FONT_DECLARE(font_puhui_basic_30_4);
LV_FONT_DECLARE(font_awesome_30_4);

namespace esp_brookesia::apps {

namespace {

constexpr char TAG[] = "XiaozhiUi";
constexpr char COMMON_FONT_PATH[] =
    BOARD_SPIFFS_MOUNT_POINT "/xiaozhi/font.bin";
constexpr size_t MAX_COMMON_FONT_SIZE = 4 * 1024 * 1024;
constexpr int TOP_BAR_HEIGHT = 54;
constexpr int SUBTITLE_SINGLE_HEIGHT = 62;
constexpr int SUBTITLE_MULTILINE_HEIGHT = 132;
constexpr int EMOJI_BOX_SIZE = 96;
constexpr int EMOJI_CENTER_OFFSET_Y = -18;
constexpr int CUSTOM_EMOJI_MAX_SIZE = 200;
constexpr int SUBTITLE_EMOJI_GAP = 12;
constexpr int ACTIVATION_BOX_HEIGHT = 250;
constexpr int ACTIVATION_BOX_OFFSET_Y = -18;

struct EmotionGlyph {
    const char *name;
    const char *utf8;
};

constexpr EmotionGlyph EMOTION_GLYPHS[] = {
    {"neutral", "\xF0\x9F\x98\xB6"},
    {"happy", "\xF0\x9F\x99\x82"},
    {"laughing", "\xF0\x9F\x98\x86"},
    {"funny", "\xF0\x9F\x98\x82"},
    {"sad", "\xF0\x9F\x98\x94"},
    {"angry", "\xF0\x9F\x98\xA0"},
    {"crying", "\xF0\x9F\x98\xAD"},
    {"loving", "\xF0\x9F\x98\x8D"},
    {"embarrassed", "\xF0\x9F\x98\xB3"},
    {"surprised", "\xF0\x9F\x98\xAF"},
    {"shocked", "\xF0\x9F\x98\xB1"},
    {"thinking", "\xF0\x9F\xA4\x94"},
    {"winking", "\xF0\x9F\x98\x89"},
    {"cool", "\xF0\x9F\x98\x8E"},
    {"relaxed", "\xF0\x9F\x98\x8C"},
    {"delicious", "\xF0\x9F\xA4\xA4"},
    {"kissy", "\xF0\x9F\x98\x98"},
    {"confident", "\xF0\x9F\x98\x8F"},
    {"sleepy", "\xF0\x9F\x98\xB4"},
    {"silly", "\xF0\x9F\x98\x9C"},
    {"confused", "\xF0\x9F\x99\x84"},
};

struct ThemePalette {
    uint32_t background;
    uint32_t bar;
    uint32_t text;
    uint32_t secondary_text;
    uint32_t network_ready;
    uint32_t network_down;
};

constexpr ThemePalette DARK_THEME = {0x101417, 0x1B2227, 0xF2F5F7, 0xA8B1B8, 0xE6E6E6, 0x6F7A80};
constexpr ThemePalette LIGHT_THEME = {0xFFFFFF, 0xFFFFFF, 0x000000, 0x5A5A5A, 0x202020, 0x8A8A8A};

void prepareTransparentObject(lv_obj_t *object)
{
    lv_obj_remove_style_all(object);
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    // Only the explicit tap targets below are clickable.
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
}

void setLabelTextIfChanged(lv_obj_t *label, const char *text)
{
    if (!label) {
        return;
    }
    const char *value = text ? text : "";
    const char *current = lv_label_get_text(label);
    if (!current || strcmp(current, value) != 0) {
        lv_label_set_text(label, value);
    }
}

void setHidden(lv_obj_t *object, bool hidden)
{
    if (!object || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN) == hidden) {
        return;
    }

    if (hidden) {
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace

XiaozhiUi::~XiaozhiUi()
{
    destroy();
    _emoji_font = nullptr;
    releaseTextFont();
    _text_font = &font_puhui_basic_30_4;
}

bool XiaozhiUi::preload()
{
    _text_font = &font_puhui_basic_30_4;
    if (_cbin_font) {
        _text_font = _cbin_font;
    } else {
        (void)loadTextFont();
    }
    if (!_emoji_font) {
        if (board_display_lock(-1)) {
            _emoji_font = font_emoji_64_init();
            board_display_unlock();
        } else {
            ESP_LOGW(TAG, "Unable to lock LVGL while loading the emoji font");
        }
    }
    return true;
}

bool XiaozhiUi::create(
    lv_obj_t *parent,
    ActionCallback action_callback,
    void *action_context,
    bool multiline_subtitle
)
{
    if (!parent) {
        return false;
    }

    destroyView();
    _action_callback = action_callback;
    _action_context = action_context;
    _text_font = _cbin_font ? _cbin_font : &font_puhui_basic_30_4;

    // TODO(theme): loadTheme() always returns light, so DARK_THEME is unused.
    const ThemePalette &theme =
        settings_storage::loadTheme() == settings_storage::THEME_LIGHT ? LIGHT_THEME : DARK_THEME;
    _palette = {theme.background, theme.bar, theme.text, theme.secondary_text, theme.network_ready,
                theme.network_down};

    const lv_image_dsc_t *skin_image = nullptr;
#if XIAOZHI_THEME_ENABLED  // TODO(theme)
    // Skin from the console's custom theme package, when one is installed.
    _package = xiaozhi_assets::load();
    if (const auto *package = _package) {
        const auto &skin = settings_storage::loadTheme() == settings_storage::THEME_LIGHT ? package->light
                                                                                         : package->dark;
        if (package->text_font) {
            _text_font = package->text_font;
        }
        if (skin.has_text_color) {
            _palette.text = skin.text_color;
        }
        if (skin.has_background_color) {
            _palette.background = skin.background_color;
        }
        skin_image = skin.background_image;
    }
#endif

    _root = lv_obj_create(parent);
    prepareTransparentObject(_root);
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_align(_root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(_root, lv_color_hex(_palette.background), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(_root, lv_color_hex(_palette.text), LV_PART_MAIN);
    lv_obj_set_style_text_font(_root, _text_font, LV_PART_MAIN);

    // The package's background image sits under every other element.
    if (skin_image) {
        lv_obj_t *background = lv_image_create(_root);
        lv_image_set_src(background, skin_image);
        lv_obj_align(background, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_clear_flag(background, LV_OBJ_FLAG_CLICKABLE);
    }


    _top_bar = lv_obj_create(_root);
    prepareTransparentObject(_top_bar);
    lv_obj_set_size(_top_bar, lv_pct(100), TOP_BAR_HEIGHT);
    lv_obj_align(_top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(_top_bar, lv_color_hex(_palette.bar), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_top_bar, LV_OPA_50, LV_PART_MAIN);

    _network_label = lv_label_create(_top_bar);
    lv_obj_set_style_text_font(_network_label, &font_awesome_30_4, LV_PART_MAIN);
    lv_obj_set_style_text_color(_network_label, lv_color_hex(_palette.network_down), LV_PART_MAIN);
    lv_obj_align(_network_label, LV_ALIGN_LEFT_MID, 16, 0);

    _status_label = lv_label_create(_top_bar);
    lv_obj_set_width(_status_label, lv_pct(74));
    lv_label_set_long_mode(_status_label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(_status_label, _text_font, LV_PART_MAIN);
    lv_obj_align(_status_label, LV_ALIGN_CENTER, 0, 0);

    _emoji_box = lv_obj_create(_root);
    prepareTransparentObject(_emoji_box);
    lv_obj_set_size(_emoji_box, EMOJI_BOX_SIZE, EMOJI_BOX_SIZE);
    lv_obj_align(_emoji_box, LV_ALIGN_CENTER, 0, EMOJI_CENTER_OFFSET_Y);

    _emoji_label = lv_label_create(_emoji_box);
    lv_obj_set_style_text_color(_emoji_label, lv_color_hex(_palette.text), LV_PART_MAIN);
    lv_obj_center(_emoji_label);

    _activation_box = lv_obj_create(_root);
    prepareTransparentObject(_activation_box);
    lv_obj_set_size(_activation_box, lv_pct(88), ACTIVATION_BOX_HEIGHT);
    lv_obj_set_flex_flow(_activation_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        _activation_box,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_set_style_pad_row(_activation_box, 18, LV_PART_MAIN);
    lv_obj_align(_activation_box, LV_ALIGN_CENTER, 0, ACTIVATION_BOX_OFFSET_Y);
    lv_obj_add_flag(_activation_box, LV_OBJ_FLAG_HIDDEN);

    _activation_code_label = lv_label_create(_activation_box);
    lv_obj_set_width(_activation_code_label, lv_pct(100));
    lv_obj_set_style_text_align(_activation_code_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(_activation_code_label, _text_font, LV_PART_MAIN);

    _activation_message_label = lv_label_create(_activation_box);
    lv_obj_set_width(_activation_message_label, lv_pct(100));
    lv_obj_set_height(_activation_message_label, 150);
    lv_label_set_long_mode(_activation_message_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(_activation_message_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(_activation_message_label, _text_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(
        _activation_message_label,
        lv_color_hex(_palette.secondary_text),
        LV_PART_MAIN
    );

    // The emoji, or the activation code shown in its place, is the tap target.
    if (_action_callback) {
        lv_obj_t *tap_targets[] = {_emoji_box, _activation_box};
        for (lv_obj_t *target : tap_targets) {
            lv_obj_add_flag(target, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(target, tapCallback, LV_EVENT_CLICKED, this);
        }
    }

    // The reply text sits directly below the emoji.
    const int subtitle_height =
        multiline_subtitle ? SUBTITLE_MULTILINE_HEIGHT : SUBTITLE_SINGLE_HEIGHT;
    _subtitle_bar = lv_obj_create(_root);
    prepareTransparentObject(_subtitle_bar);
    lv_obj_set_size(_subtitle_bar, lv_pct(100), subtitle_height);
    lv_obj_align(
        _subtitle_bar,
        LV_ALIGN_CENTER,
        0,
        EMOJI_CENTER_OFFSET_Y + EMOJI_BOX_SIZE / 2 + SUBTITLE_EMOJI_GAP + subtitle_height / 2
    );
    lv_obj_set_style_bg_color(_subtitle_bar, lv_color_hex(_palette.bar), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_subtitle_bar, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(_subtitle_bar, 16, LV_PART_MAIN);
    lv_obj_add_flag(_subtitle_bar, LV_OBJ_FLAG_HIDDEN);

    _subtitle_label = lv_label_create(_subtitle_bar);
    lv_obj_set_width(_subtitle_label, lv_pct(100));
    if (multiline_subtitle) {
        lv_obj_set_height(_subtitle_label, SUBTITLE_MULTILINE_HEIGHT - 20);
    }
    lv_label_set_long_mode(
        _subtitle_label,
        multiline_subtitle ? LV_LABEL_LONG_MODE_WRAP : LV_LABEL_LONG_MODE_SCROLL_CIRCULAR
    );
    lv_obj_set_style_text_align(_subtitle_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(_subtitle_label, _text_font, LV_PART_MAIN);
    lv_obj_align(_subtitle_label, LV_ALIGN_CENTER, 0, 0);

    setNetworkReady(false);
    setStatus("");
    setEmotion("neutral");
    clearChatMessages();
    return true;
}

void XiaozhiUi::destroy()
{
    destroyView();
}

void XiaozhiUi::destroyView()
{
#if XIAOZHI_THEME_ENABLED
    if (_active_gif) {
        _active_gif->SetFrameCallback(nullptr);
        _active_gif = nullptr;
    }
    _gifs.clear();
#endif
    if (_root) {
        lv_obj_delete(_root);
    }

    _root = nullptr;
    _top_bar = nullptr;
    _network_label = nullptr;
    _status_label = nullptr;
    _emoji_box = nullptr;
    _emoji_label = nullptr;
    _emoji_image = nullptr;
#if XIAOZHI_THEME_ENABLED
    _package = nullptr;
#endif
    _activation_box = nullptr;
    _activation_code_label = nullptr;
    _activation_message_label = nullptr;
    _subtitle_bar = nullptr;
    _subtitle_label = nullptr;
    _network_state_valid = false;
    _network_ready = false;
    _activation_state_valid = false;
    _activation_visible = false;
    _emotion_name[0] = '\0';
}

void XiaozhiUi::setNetworkReady(bool ready)
{
    if (!_network_label) {
        return;
    }
    if (_network_state_valid && _network_ready == ready) {
        return;
    }
    lv_label_set_text(
        _network_label,
        ready ? FONT_AWESOME_WIFI : FONT_AWESOME_WIFI_SLASH
    );
    lv_obj_set_style_text_color(
        _network_label,
        lv_color_hex(ready ? _palette.network_ready : _palette.network_down),
        LV_PART_MAIN
    );
    _network_ready = ready;
    _network_state_valid = true;
}

void XiaozhiUi::setStatus(const char *status)
{
    setLabelTextIfChanged(_status_label, status);
}

void XiaozhiUi::setChatMessage(const char *role, const char *text)
{
    (void)role;
    if (!_subtitle_label || !_subtitle_bar) {
        return;
    }

    const char *value = text ? text : "";
    setLabelTextIfChanged(_subtitle_label, value);
    setHidden(_subtitle_bar, value[0] == '\0');
}

void XiaozhiUi::clearChatMessages()
{
    setChatMessage("system", "");
}

void XiaozhiUi::setEmotion(const char *emotion)
{
    if (!_emoji_label) {
        return;
    }

    const char *name = emotion && emotion[0] ? emotion : "neutral";
    if (strcmp(_emotion_name, name) == 0) {
        return;
    }

#if XIAOZHI_THEME_ENABLED  // TODO(theme)
    // A custom emoji from the theme package replaces the built-in emoji font.
    if (const auto *custom = _package ? _package->findEmoji(name) : nullptr) {
        if (!_emoji_image) {
            _emoji_image = lv_image_create(_emoji_box);
            lv_obj_center(_emoji_image);
            lv_obj_clear_flag(_emoji_image, LV_OBJ_FLAG_CLICKABLE);
        }
        if (_active_gif) {
            _active_gif->Stop();
            _active_gif->SetFrameCallback(nullptr);
            _active_gif = nullptr;
        }

        // GIFs are played by the decoder ported from upstream (it handles transparent frames);
        // its canvas is an ARGB8888 image that is redrawn every frame. PNGs are decoded by LVGL.
        const lv_image_dsc_t *source = &custom->dsc;
        LvglGif *gif = nullptr;
        int image_w = 0;
        int image_h = 0;
        if (custom->is_gif) {
            auto &slot = _gifs[name];
            if (!slot) {
                slot = std::make_unique<LvglGif>(&custom->dsc);
            }
            if (!slot->IsLoaded()) {
                ESP_LOGW(TAG, "Custom emoji %s is not a valid GIF", name);
                return;
            }
            gif = slot.get();
            source = gif->image_dsc();
            image_w = gif->width();
            image_h = gif->height();
        } else {
            lv_image_header_t header = {};
            if (lv_image_decoder_get_info(&custom->dsc, &header) == LV_RESULT_OK) {
                image_w = header.w;
                image_h = header.h;
            }
        }
        lv_image_set_src(_emoji_image, source);

        // The emoji box is 96 px; a larger custom image would be clipped, so grow the box to the
        // image (scaled down to CUSTOM_EMOJI_MAX_SIZE) and let it draw outside its bounds.
        int box_size = EMOJI_BOX_SIZE;
        int scale = LV_SCALE_NONE;
        if (image_w > 0 && image_h > 0) {
            const int largest = image_w > image_h ? image_w : image_h;
            if (largest > CUSTOM_EMOJI_MAX_SIZE) {
                scale = CUSTOM_EMOJI_MAX_SIZE * LV_SCALE_NONE / largest;
            }
            box_size = largest * scale / LV_SCALE_NONE;
            if (box_size < EMOJI_BOX_SIZE) {
                box_size = EMOJI_BOX_SIZE;
            }
        }
        ESP_LOGI(TAG, "Custom emoji %s: %dx%d %s, %u bytes, box=%d scale=%d", name, image_w, image_h,
                 custom->is_gif ? "GIF" : "PNG", static_cast<unsigned>(custom->dsc.data_size), box_size, scale);
        lv_image_set_scale(_emoji_image, scale);
        lv_obj_add_flag(_emoji_box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_size(_emoji_box, box_size, box_size);
        lv_obj_align(_emoji_box, LV_ALIGN_CENTER, 0, EMOJI_CENTER_OFFSET_Y);
        lv_obj_center(_emoji_image);

        if (gif) {
            lv_obj_t *image = _emoji_image;
            gif->SetFrameCallback([image]() { lv_obj_invalidate(image); });
            gif->Start();
            _active_gif = gif;
        }
        setHidden(_emoji_label, true);
        setHidden(_emoji_image, false);
        snprintf(_emotion_name, sizeof(_emotion_name), "%s", name);
        return;
    }
    if (_active_gif) {
        _active_gif->Stop();
        _active_gif->SetFrameCallback(nullptr);
        _active_gif = nullptr;
    }
#endif
    setHidden(_emoji_image, true);
    setHidden(_emoji_label, false);

    const char *emoji = emojiForEmotion(name);
    if (_emoji_font && emoji) {
        lv_obj_set_style_text_font(_emoji_label, _emoji_font, LV_PART_MAIN);
        lv_label_set_text(_emoji_label, emoji);
        snprintf(_emotion_name, sizeof(_emotion_name), "%s", name);
        return;
    }

    const char *icon = font_awesome_get_utf8(name);
    lv_obj_set_style_text_font(_emoji_label, &font_awesome_30_4, LV_PART_MAIN);
    lv_label_set_text(_emoji_label, icon ? icon : FONT_AWESOME_MICROCHIP_AI);
    snprintf(_emotion_name, sizeof(_emotion_name), "%s", name);
}

void XiaozhiUi::setActivation(const char *code, const char *message, bool visible)
{
    if (!_activation_box || !_emoji_box) {
        return;
    }

    setLabelTextIfChanged(_activation_code_label, code);
    setLabelTextIfChanged(_activation_message_label, message);
    if (!_activation_state_valid || _activation_visible != visible) {
        setHidden(_emoji_box, visible);
        setHidden(_activation_box, !visible);
        _activation_visible = visible;
        _activation_state_valid = true;
    }
}

bool XiaozhiUi::loadTextFont()
{
    if (_cbin_font) {
        _text_font = _cbin_font;
        return true;
    }
    FILE *file = fopen(COMMON_FONT_PATH, "rb");
    if (!file) {
        ESP_LOGW(TAG, "Common font is unavailable: %s", COMMON_FONT_PATH);
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long file_size = ftell(file);
    if (file_size <= 0 || static_cast<size_t>(file_size) > MAX_COMMON_FONT_SIZE ||
            fseek(file, 0, SEEK_SET) != 0) {
        ESP_LOGW(TAG, "Common font has an invalid size: %ld", file_size);
        fclose(file);
        return false;
    }

    size_t size = static_cast<size_t>(file_size);
    auto *data = static_cast<uint8_t *>(
                     heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                 );
    if (!data) {
        ESP_LOGW(TAG, "No PSRAM for common font (%u bytes)", static_cast<unsigned>(size));
        fclose(file);
        return false;
    }

    size_t read_size = fread(data, 1, size, file);
    fclose(file);
    if (read_size != size) {
        ESP_LOGW(TAG, "Common font read was incomplete (%u/%u)",
                 static_cast<unsigned>(read_size), static_cast<unsigned>(size));
        heap_caps_free(data);
        return false;
    }

    if (!board_display_lock(-1)) {
        ESP_LOGW(TAG, "Unable to lock LVGL while decoding the common font");
        heap_caps_free(data);
        return false;
    }
    lv_font_t *font = cbin_font_create(data);
    board_display_unlock();

    if (!font) {
        ESP_LOGW(TAG, "Common font could not be decoded");
        heap_caps_free(data);
        return false;
    }

    _font_data = data;
    _cbin_font = font;
    _text_font = font;
    ESP_LOGI(TAG, "Loaded common font from PSRAM (%u bytes)", static_cast<unsigned>(size));
    return true;
}

void XiaozhiUi::releaseTextFont()
{
    if (_cbin_font && !board_display_lock(-1)) {
        ESP_LOGE(TAG, "Unable to lock LVGL while releasing the common font");
        return;
    }
    if (_cbin_font) {
        cbin_font_delete(_cbin_font);
        _cbin_font = nullptr;
        board_display_unlock();
    }
    if (_font_data) {
        heap_caps_free(_font_data);
        _font_data = nullptr;
    }
}

void XiaozhiUi::tapCallback(lv_event_t *event)
{
    auto *ui = static_cast<XiaozhiUi *>(lv_event_get_user_data(event));
    if (ui && ui->_action_callback) {
        ui->_action_callback(ui->_action_context);
    }
}

const char *XiaozhiUi::emojiForEmotion(const char *emotion)
{
    if (!emotion) {
        return nullptr;
    }
    for (const auto &entry : EMOTION_GLYPHS) {
        if (strcmp(entry.name, emotion) == 0) {
            return entry.utf8;
        }
    }
    return nullptr;
}

} // namespace esp_brookesia::apps
