#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:CanTool"
#include "esp_lib_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include "CanTool.hpp"

#define APP_NAME "CAN"
#define BITRATE_OPTIONS "125 kbit/s\n250 kbit/s\n500 kbit/s\n1 Mbit/s"

LV_IMG_DECLARE(img_app_can_tool);

namespace esp_brookesia::apps {

namespace {

constexpr uint32_t COLOR_SCREEN = 0x0B0F14;
constexpr uint32_t COLOR_CARD = 0x151B23;
constexpr uint32_t COLOR_SURFACE = 0x090D12;
constexpr uint32_t COLOR_BORDER = 0x334155;
constexpr uint32_t COLOR_PRIMARY = 0x3B82F6;
constexpr uint32_t COLOR_PRIMARY_DARK = 0x1D4ED8;
constexpr uint32_t COLOR_TEXT = 0xF8FAFC;
constexpr uint32_t COLOR_TEXT_MUTED = 0x94A3B8;
constexpr uint32_t COLOR_WARNING = 0xF59E0B;
constexpr uint32_t COLOR_ERROR = 0xEF5350;
constexpr uint32_t COLOR_SUCCESS = 0x22C55E;
constexpr uint32_t COLOR_RX = 0x38BDF8;
constexpr uint32_t COLOR_TX = 0xA78BFA;
constexpr uint32_t UI_REFRESH_PERIOD_MS = 20;
constexpr char TX_DEFAULT_ID[] = "0x123";
constexpr char TX_DEFAULT_DATA[] = "00 11 22 33 44 55 66 77";
constexpr size_t UI_DRAIN_CHUNK = 16;
constexpr size_t UI_DRAIN_BUDGET = 256;
constexpr size_t FRAME_RENDER_MAX_LINES = 24;
constexpr uint8_t FRAME_RENDER_TICKS = 5;
constexpr uint8_t STATUS_POLL_TICKS = 15;

static const int bitrate_values[] = {125000, 250000, 500000, 1000000};
constexpr size_t BITRATE_OPTION_COUNT =
    sizeof(bitrate_values) / sizeof(bitrate_values[0]);

static const char *hex_kb_map[] = {
    "1", "2", "3", "A", "B", "\n",
    "4", "5", "6", "C", "D", "\n",
    "7", "8", "9", "E", "F", "\n",
    "0", " ", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t hex_kb_ctrl[] = {
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
    (lv_buttonmatrix_ctrl_t)1, (lv_buttonmatrix_ctrl_t)1,
};

void style_transparent(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

void style_card(lv_obj_t *obj, int radius = 18)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_shadow_width(obj, 12, 0);
    lv_obj_set_style_shadow_ofs_y(obj, 4, 0);
}

void style_button(lv_obj_t *obj, bool primary = false)
{
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(
        obj, lv_color_hex(primary ? COLOR_PRIMARY : COLOR_BORDER), 0
    );
    lv_obj_set_style_bg_color(
        obj, lv_color_hex(primary ? COLOR_PRIMARY : COLOR_CARD), 0
    );
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_PRIMARY_DARK), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_opa(obj, LV_OPA_40, LV_STATE_DISABLED);
}

lv_obj_t *make_label(
    lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color
)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

lv_obj_t *make_button(
    lv_obj_t *parent, const char *text, int width, int height, bool primary = false
)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, width, height);
    style_button(button, primary);
    lv_obj_t *label = make_label(button, text, &lv_font_montserrat_22, COLOR_TEXT);
    lv_obj_center(label);
    return button;
}

// Transparent, non-scrollable flex container sized to its content by default.
lv_obj_t *make_box(lv_obj_t *parent, lv_flex_flow_t flow, int gap)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    style_transparent(box);
    lv_obj_set_style_pad_column(box, gap, 0);
    lv_obj_set_style_pad_row(box, gap, 0);
    lv_obj_set_flex_flow(box, flow);
    lv_obj_set_flex_align(
        box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

void set_checked(lv_obj_t *obj, bool checked)
{
    if (obj == nullptr) {
        return;
    }
    if (checked) {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
    }
}

void set_label_text_if_changed(lv_obj_t *label, const char *text)
{
    if (label == nullptr || text == nullptr) {
        return;
    }
    const char *current = lv_label_get_text(label);
    if (current == nullptr || std::strcmp(current, text) != 0) {
        lv_label_set_text(label, text);
    }
}

int bitrate_to_index(int bitrate)
{
    for (size_t i = 0; i < BITRATE_OPTION_COUNT; i++) {
        if (bitrate_values[i] == bitrate) {
            return static_cast<int>(i);
        }
    }
    return 2;
}

int hex_value(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

constexpr bool can_id_matches_filter(
    uint32_t frame_id, uint32_t filter_id, uint32_t filter_mask
)
{
    return (frame_id & filter_mask) == (filter_id & filter_mask);
}

static_assert(can_id_matches_filter(0x123, 0x123, 0x1FFFFFFF));
static_assert(!can_id_matches_filter(0x124, 0x123, 0x1FFFFFFF));
static_assert(can_id_matches_filter(0x12F, 0x120, 0x7F0));

bool parse_can_id(const char *text, bool extended, uint32_t *value_out = nullptr)
{
    if (text == nullptr) {
        return false;
    }

    while (*text != '\0' && std::isspace(static_cast<unsigned char>(*text))) {
        text++;
    }
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }

    uint32_t value = 0;
    int digits = 0;
    while (*text != '\0') {
        const int nibble = hex_value(*text);
        if (nibble < 0) {
            break;
        }
        if (digits >= 8) {
            return false;
        }
        value = (value << 4) | static_cast<uint32_t>(nibble);
        digits++;
        text++;
    }
    while (*text != '\0' && std::isspace(static_cast<unsigned char>(*text))) {
        text++;
    }

    const uint32_t maximum = extended ? 0x1FFFFFFF : 0x7FF;
    const bool valid = digits > 0 && *text == '\0' && value <= maximum;
    if (valid && value_out != nullptr) {
        *value_out = value;
    }
    return valid;
}

int parse_can_data(const char *text, uint8_t *data_out = nullptr)
{
    if (text == nullptr) {
        return -1;
    }

    int count = 0;
    while (*text != '\0') {
        while (*text != '\0' &&
               (std::isspace(static_cast<unsigned char>(*text)) || *text == ',')) {
            text++;
        }
        if (*text == '\0') {
            break;
        }

        uint8_t value = 0;
        int digits = 0;
        while (hex_value(*text) >= 0) {
            value = static_cast<uint8_t>(
                (value << 4) | static_cast<uint8_t>(hex_value(*text))
            );
            digits++;
            text++;
        }
        if (digits != 2) {
            return -1;
        }
        if (*text != '\0' &&
            !std::isspace(static_cast<unsigned char>(*text)) && *text != ',') {
            return -1;
        }

        if (data_out != nullptr && count < 8) {
            data_out[count] = value;
        }
        count++;
        if (count > 8) {
            return -2;
        }
    }
    return count;
}

void style_textarea(lv_obj_t *textarea)
{
    lv_obj_set_style_bg_color(textarea, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(textarea, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_color(textarea, lv_color_hex(COLOR_PRIMARY), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(textarea, 1, 0);
    lv_obj_set_style_radius(textarea, 12, 0);
    lv_obj_set_style_text_color(textarea, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_color(
        textarea, lv_color_hex(COLOR_TEXT_MUTED), LV_PART_TEXTAREA_PLACEHOLDER
    );
    lv_obj_set_style_text_font(textarea, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(
        textarea, &lv_font_montserrat_22, LV_PART_TEXTAREA_PLACEHOLDER
    );
}

} // namespace

CanTool *CanTool::_instance = nullptr;

CanTool *CanTool::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new CanTool(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

CanTool::CanTool(bool use_status_bar, bool use_navigation_bar):
    App(APP_NAME, &img_app_can_tool, true, use_status_bar, use_navigation_bar)
{
}

CanTool::~CanTool()
{
    ESP_UTILS_LOGD("Destroy(@0x%p)", this);
}

bool CanTool::run(void)
{
    ESP_UTILS_LOGD("Run(@0x%p)", this);
    _config = can_tool_config_load();
    _service_status = {};
    clearFrameHistory();
    _frames_frozen = false;
    _advanced_filter_enabled = false;
    _advanced_filter_id = 0x123;
    _advanced_filter_mask = 0x1FFFFFFF;
    _tx_extended = false;
    _tx_remote = false;
    _ui_paused = false;
    _frame_render_dirty = false;
    _frame_render_divider = 0;
    _status_poll_divider = 0;

    const auto visual_area = getVisualArea();
    int width = lv_area_get_width(&visual_area);
    int height = lv_area_get_height(&visual_area);
    if (width <= 0 || height <= 0) {
        width = 720;
        height = 1280;
    }

    _screen = lv_screen_active();
    lv_obj_set_style_bg_color(_screen, lv_color_hex(COLOR_SCREEN), 0);
    createUi(_screen, width, height);
    lv_obj_add_event_cb(
        _screen, screen_size_changed_event_cb, LV_EVENT_SIZE_CHANGED, this
    );
    restartService();
    _ui_timer = lv_timer_create(ui_refresh_timer_cb, UI_REFRESH_PERIOD_MS, this);
    return true;
}

void CanTool::createUi(lv_obj_t *screen, int width, int height)
{
    _landscape = width > height;
    const int pad_x = _landscape ? 16 : std::max(18, width * 4 / 100);
    const int pad_y = _landscape ? 12 : std::max(14, height * 2 / 100);
    _layout_w = width;
    _layout_h = height;
    _content_w = width - 2 * pad_x;
    // The stacked portrait layout has its own fallbacks for short / narrow
    // screens; the landscape layout fits without them.
    _compact_layout = !_landscape && height < 900;
    _narrow_layout = !_landscape && _content_w < 620;
    _root_pad = pad_y;
    _keyboard_h = height * 40 / 100;

    _root = lv_obj_create(screen);
    lv_obj_set_size(_root, _content_w, height - 2 * pad_y);
    if (_landscape) {
        // Anchored to the top so it can shrink above the keyboard.
        lv_obj_align(_root, LV_ALIGN_TOP_MID, 0, pad_y);
    } else {
        lv_obj_center(_root);
    }
    style_transparent(_root);
    lv_obj_set_style_pad_row(_root, (_landscape || _compact_layout) ? 10 : 14, 0);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    if (_compact_layout) {
        lv_obj_set_scroll_dir(_root, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
    } else {
        lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
        lv_obj_remove_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    }

    if (_landscape) {
        createLandscapeUi(_root);
    } else {
        createHeader(_root);
        createConnectionCard(_root);
        createBusStatus(_root);
        createFramesSection(_root);
        createTransmitSection(_root);
    }

    updateBitrateSummary();
    updateConfigWidgets();
    updateDlcLabel();
    updateConnectionStatus();
    renderFrames();
}

void CanTool::rebuildUi(int width, int height)
{
    if (_screen == nullptr || width <= 0 || height <= 0 ||
        (width == _layout_w && height == _layout_h)) {
        return;
    }

    ESP_UTILS_LOGI("Relayout UI: %dx%d -> %dx%d", _layout_w, _layout_h, width, height);
    deleteUiObjects();
    createUi(_screen, width, height);
}

void CanTool::deleteUiObjects()
{
    if (_keyboard != nullptr) {
        lv_obj_delete(_keyboard);
    }
    if (_settings_overlay != nullptr) {
        lv_obj_delete(_settings_overlay);
    }
    if (_filter_overlay != nullptr) {
        lv_obj_delete(_filter_overlay);
    }
    if (_root != nullptr) {
        lv_obj_delete(_root);
    }
    clearUiPointers();
}

void CanTool::clearUiPointers()
{
    _root = nullptr;
    _top_bar = nullptr;
    _status_dot = nullptr;
    _status_label = nullptr;
    for (auto &button : _operating_mode_buttons) {
        button = nullptr;
    }
    _bitrate_summary_label = nullptr;
    _bus_state_label = nullptr;
    _tx_count_label = nullptr;
    _rx_count_label = nullptr;
    _tec_label = nullptr;
    _rec_label = nullptr;
    for (auto &button : _frame_filter_buttons) {
        button = nullptr;
    }
    _advanced_filter_button = nullptr;
    _advanced_filter_label = nullptr;
    _freeze_button = nullptr;
    _freeze_label = nullptr;
    _frame_count_label = nullptr;
    for (auto &label : _frame_column_labels) {
        label = nullptr;
    }
    _frames_empty_label = nullptr;
    _tx_id_textarea = nullptr;
    _tx_data_textarea = nullptr;
    for (auto &button : _tx_format_buttons) {
        button = nullptr;
    }
    for (auto &button : _tx_type_buttons) {
        button = nullptr;
    }
    _dlc_label = nullptr;
    _send_button = nullptr;
    _filter_overlay = nullptr;
    _filter_id_textarea = nullptr;
    _filter_mask_textarea = nullptr;
    _filter_hint_label = nullptr;
    _settings_overlay = nullptr;
    _bitrate_dropdown = nullptr;
    _keyboard = nullptr;
}

void CanTool::createHeader(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), _compact_layout ? 52 : 64);
    style_transparent(header);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        header, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    make_label(
        header, "CAN Monitor",
        _narrow_layout ? &lv_font_montserrat_30 : &lv_font_montserrat_36,
        COLOR_TEXT
    );

    lv_obj_t *status = lv_obj_create(header);
    lv_obj_set_size(status, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    style_transparent(status);
    lv_obj_set_style_pad_column(status, 10, 0);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        status, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);

    _status_dot = lv_obj_create(status);
    lv_obj_set_size(_status_dot, _narrow_layout ? 12 : 16, _narrow_layout ? 12 : 16);
    lv_obj_set_style_radius(_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_status_dot, 0, 0);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(COLOR_WARNING), 0);
    lv_obj_remove_flag(_status_dot, LV_OBJ_FLAG_SCROLLABLE);

    _status_label = make_label(
        status, "Pins NC",
        _narrow_layout ? &lv_font_montserrat_20 : &lv_font_montserrat_24,
        COLOR_WARNING
    );
}

void CanTool::createConnectionCard(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), _narrow_layout ? 220 : 174);
    style_card(card);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *mode_row = lv_obj_create(card);
    lv_obj_set_size(mode_row, LV_PCT(100), 66);
    lv_obj_set_style_bg_color(mode_row, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mode_row, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(mode_row, 1, 0);
    lv_obj_set_style_radius(mode_row, 14, 0);
    lv_obj_set_style_pad_all(mode_row, 4, 0);
    lv_obj_set_style_pad_column(mode_row, 4, 0);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        mode_row, LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);

    const char *mode_names[] = {"Normal", "Listen Only"};
    for (int i = 0; i < 2; i++) {
        _operating_mode_buttons[i] = make_button(mode_row, mode_names[i], 0, 58);
        lv_obj_set_flex_grow(_operating_mode_buttons[i], 1);
        lv_obj_add_event_cb(
            _operating_mode_buttons[i],
            operating_mode_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    lv_obj_t *summary_row = lv_obj_create(card);
    lv_obj_set_size(summary_row, LV_PCT(100), _narrow_layout ? 116 : 62);
    style_transparent(summary_row);
    lv_obj_set_style_pad_column(summary_row, 12, 0);
    lv_obj_set_style_pad_row(summary_row, 8, 0);
    lv_obj_set_flex_flow(
        summary_row, _narrow_layout ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW
    );
    lv_obj_set_flex_align(
        summary_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(summary_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *summary = lv_obj_create(summary_row);
    lv_obj_set_size(summary, _narrow_layout ? LV_PCT(100) : 0, LV_SIZE_CONTENT);
    if (!_narrow_layout) {
        lv_obj_set_flex_grow(summary, 1);
    }
    style_transparent(summary);
    lv_obj_set_style_pad_column(summary, 18, 0);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        summary, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(summary, LV_OBJ_FLAG_SCROLLABLE);

    _bitrate_summary_label = make_label(
        summary, "", &lv_font_montserrat_22, COLOR_TEXT
    );
    make_label(summary, "|", &lv_font_montserrat_22, COLOR_BORDER);
    make_label(summary, "Classic CAN", &lv_font_montserrat_22, COLOR_TEXT_MUTED);

    lv_obj_t *settings_btn = make_button(
        summary_row,
        _narrow_layout ? LV_SYMBOL_SETTINGS : LV_SYMBOL_SETTINGS "  Settings",
        _narrow_layout ? LV_PCT(100) : 168,
        56
    );
    lv_obj_add_event_cb(
        settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, this
    );
}

void CanTool::createBusStatus(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), _narrow_layout ? 176 : 96);
    style_card(card, 16);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_column(card, 4, 0);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_set_flex_flow(
        card, _narrow_layout ? LV_FLEX_FLOW_ROW_WRAP : LV_FLEX_FLOW_ROW
    );
    lv_obj_set_flex_align(
        card, LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY
    );
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    auto add_status = [&](const char *name, const char *value, uint32_t color,
                          int grow, lv_obj_t **value_label) {
        lv_obj_t *item = lv_obj_create(card);
        lv_obj_set_size(item, 0, _narrow_layout ? 72 : 72);
        lv_obj_set_flex_grow(item, grow);
        style_transparent(item);
        lv_obj_set_style_pad_row(item, 4, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(
            item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
        );
        lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        make_label(item, name, &lv_font_montserrat_20, COLOR_TEXT_MUTED);
        *value_label = make_label(item, value, &lv_font_montserrat_22, color);
    };

    add_status("STATE", "Stopped", COLOR_WARNING, 2, &_bus_state_label);
    add_status("TX", "0", COLOR_TX, 1, &_tx_count_label);
    add_status("RX", "0", COLOR_RX, 1, &_rx_count_label);
    add_status("TEC", "0", COLOR_TEXT, 1, &_tec_label);
    add_status("REC", "0", COLOR_TEXT, 1, &_rec_label);
}

void CanTool::createFramesSection(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(100));
    if (_compact_layout) {
        lv_obj_set_height(card, 330);
    } else {
        lv_obj_set_height(card, 0);
        lv_obj_set_flex_grow(card, 1);
    }
    style_card(card);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *heading = lv_obj_create(card);
    lv_obj_set_size(heading, LV_PCT(100), 38);
    style_transparent(heading);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        heading, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(heading, LV_OBJ_FLAG_SCROLLABLE);
    make_label(heading, "FRAMES", &lv_font_montserrat_28, COLOR_PRIMARY);
    _frame_count_label = make_label(
        heading, "0 frames", &lv_font_montserrat_20, COLOR_TEXT_MUTED
    );

    lv_obj_t *toolbar = lv_obj_create(card);
    lv_obj_set_size(toolbar, LV_PCT(100), _narrow_layout ? 120 : 58);
    style_transparent(toolbar);
    lv_obj_set_style_pad_column(toolbar, 10, 0);
    lv_obj_set_style_pad_row(toolbar, 6, 0);
    lv_obj_set_flex_flow(
        toolbar, _narrow_layout ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW
    );
    lv_obj_set_flex_align(
        toolbar, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *filters = lv_obj_create(toolbar);
    lv_obj_set_size(filters, _narrow_layout ? LV_PCT(100) : 270, 56);
    style_transparent(filters);
    lv_obj_set_style_pad_column(filters, 6, 0);
    lv_obj_set_flex_flow(filters, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        filters, LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(filters, LV_OBJ_FLAG_SCROLLABLE);

    const char *filter_names[] = {"All", "STD", "EXT"};
    for (int i = 0; i < 3; i++) {
        _frame_filter_buttons[i] = make_button(filters, filter_names[i], 0, 52);
        lv_obj_set_flex_grow(_frame_filter_buttons[i], 1);
        lv_obj_add_event_cb(
            _frame_filter_buttons[i],
            frame_filter_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    lv_obj_t *actions = lv_obj_create(toolbar);
    lv_obj_set_size(actions, _narrow_layout ? LV_PCT(100) : 310, 56);
    style_transparent(actions);
    lv_obj_set_style_pad_column(actions, 6, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        actions, LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

    _advanced_filter_button = make_button(actions, "Filter", 0, 52);
    lv_obj_set_flex_grow(_advanced_filter_button, 1);
    _advanced_filter_label = lv_obj_get_child(_advanced_filter_button, 0);
    lv_obj_add_event_cb(
        _advanced_filter_button,
        advanced_filter_btn_event_cb,
        LV_EVENT_CLICKED,
        this
    );

    _freeze_button = make_button(actions, "Freeze", 0, 52);
    lv_obj_set_flex_grow(_freeze_button, 1);
    _freeze_label = lv_obj_get_child(_freeze_button, 0);
    lv_obj_add_event_cb(
        _freeze_button, freeze_btn_event_cb, LV_EVENT_CLICKED, this
    );

    lv_obj_t *clear_button = make_button(actions, "Clear", 0, 52);
    lv_obj_set_flex_grow(clear_button, 1);
    lv_obj_add_event_cb(
        clear_button, clear_btn_event_cb, LV_EVENT_CLICKED, this
    );

    createFrameTable(card, _content_w);
}

void CanTool::createFrameTable(lv_obj_t *card, int table_w)
{
    lv_obj_t *frame_view = lv_obj_create(card);
    lv_obj_set_width(frame_view, LV_PCT(100));
    lv_obj_set_height(frame_view, 0);
    lv_obj_set_flex_grow(frame_view, 1);
    lv_obj_set_style_bg_color(frame_view, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(frame_view, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(frame_view, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(frame_view, 1, 0);
    lv_obj_set_style_radius(frame_view, 14, 0);
    lv_obj_set_style_pad_all(frame_view, 0, 0);
    lv_obj_set_flex_flow(frame_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(frame_view, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *columns = lv_obj_create(frame_view);
    lv_obj_set_size(columns, LV_PCT(100), 52);
    style_transparent(columns);
    lv_obj_set_style_bg_color(columns, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_bg_opa(columns, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(columns, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(columns, 1, 0);
    lv_obj_set_style_border_side(columns, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_left(columns, 14, 0);
    lv_obj_set_style_pad_right(columns, 14, 0);
    lv_obj_set_style_pad_column(columns, 0, 0);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        columns, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(columns, LV_OBJ_FLAG_SCROLLABLE);

    const bool wide_table = table_w >= 820;
    const bool narrow_table = table_w < 620;
    int column_widths[FRAME_COLUMN_COUNT] = {};
    if (wide_table) {
        const int widths[FRAME_COLUMN_COUNT] = {
            118, 56, 70, 70, 154, 56, 0
        };
        std::copy_n(widths, FRAME_COLUMN_COUNT, column_widths);
    } else if (narrow_table) {
        const int widths[FRAME_COLUMN_COUNT] = {
            72, 34, 44, 46, 108, 34, 0
        };
        std::copy_n(widths, FRAME_COLUMN_COUNT, column_widths);
    } else {
        const int widths[FRAME_COLUMN_COUNT] = {
            92, 42, 54, 58, 130, 42, 0
        };
        std::copy_n(widths, FRAME_COLUMN_COUNT, column_widths);
    }
    const char *column_names[FRAME_COLUMN_COUNT] = {
        "TIME", "DIR", "FMT", "TYPE", "ID", "LEN", "DATA (HEX)"
    };
    const lv_font_t *header_font =
        _narrow_layout ? &lv_font_montserrat_16 : &lv_font_montserrat_18;
    auto set_column_width = [&](lv_obj_t *label, size_t index) {
        const bool centered =
            index == 1 || index == 2 || index == 3 || index == 5;
        lv_obj_set_style_text_align(
            label, centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0
        );
        lv_obj_set_style_pad_left(label, centered ? 4 : 8, 0);
        lv_obj_set_style_pad_right(label, centered ? 4 : 8, 0);
        if (index + 1 < FRAME_COLUMN_COUNT) {
            lv_obj_set_style_border_color(
                label, lv_color_hex(COLOR_BORDER), 0
            );
            lv_obj_set_style_border_opa(label, LV_OPA_40, 0);
            lv_obj_set_style_border_width(label, 1, 0);
            lv_obj_set_style_border_side(label, LV_BORDER_SIDE_RIGHT, 0);
        }
        if (index + 1 == FRAME_COLUMN_COUNT) {
            lv_obj_set_width(label, 0);
            lv_obj_set_flex_grow(label, 1);
        } else {
            lv_obj_set_width(label, column_widths[index]);
        }
    };
    for (size_t i = 0; i < FRAME_COLUMN_COUNT; i++) {
        lv_obj_t *label = make_label(
            columns, column_names[i], header_font, COLOR_TEXT_MUTED
        );
        lv_obj_set_height(label, LV_PCT(100));
        lv_obj_set_style_pad_top(
            label,
            std::max<int32_t>(
                0, (50 - lv_font_get_line_height(header_font)) / 2
            ),
            0
        );
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
        set_column_width(label, i);
    }

    lv_obj_t *rows_area = lv_obj_create(frame_view);
    lv_obj_set_width(rows_area, LV_PCT(100));
    lv_obj_set_height(rows_area, 0);
    lv_obj_set_flex_grow(rows_area, 1);
    style_transparent(rows_area);
    lv_obj_set_style_pad_top(rows_area, 10, 0);
    lv_obj_set_style_pad_bottom(rows_area, 10, 0);
    lv_obj_set_style_pad_left(rows_area, 14, 0);
    lv_obj_set_style_pad_right(rows_area, 14, 0);
    lv_obj_set_style_pad_column(rows_area, 0, 0);
    lv_obj_set_flex_flow(rows_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        rows_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START
    );
    lv_obj_remove_flag(rows_area, LV_OBJ_FLAG_SCROLLABLE);

    const lv_font_t *row_font =
        _narrow_layout ? &lv_font_montserrat_14 : &lv_font_montserrat_16;
    const uint32_t column_colors[FRAME_COLUMN_COUNT] = {
        COLOR_TEXT_MUTED,
        COLOR_TEXT,
        COLOR_TEXT_MUTED,
        COLOR_TEXT_MUTED,
        COLOR_PRIMARY,
        COLOR_TEXT_MUTED,
        COLOR_TEXT,
    };
    for (size_t i = 0; i < FRAME_COLUMN_COUNT; i++) {
        _frame_column_labels[i] = make_label(
            rows_area, "", row_font, column_colors[i]
        );
        lv_obj_set_height(_frame_column_labels[i], LV_PCT(100));
        lv_label_set_long_mode(
            _frame_column_labels[i], LV_LABEL_LONG_MODE_CLIP
        );
        lv_obj_set_style_text_line_space(
            _frame_column_labels[i], _narrow_layout ? 4 : 6, 0
        );
        set_column_width(_frame_column_labels[i], i);
        lv_obj_add_flag(_frame_column_labels[i], LV_OBJ_FLAG_HIDDEN);
    }

    _frames_empty_label = make_label(
        rows_area, "No frames received", &lv_font_montserrat_22, COLOR_TEXT_MUTED
    );
    lv_obj_add_flag(_frames_empty_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_center(_frames_empty_label);
}

void CanTool::createTransmitSection(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), _narrow_layout ? 390 : 290);
    style_card(card);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    make_label(card, "TRANSMIT", &lv_font_montserrat_28, COLOR_PRIMARY);

    lv_obj_t *meta_row = lv_obj_create(card);
    lv_obj_set_size(meta_row, LV_PCT(100), _narrow_layout ? 166 : 72);
    style_transparent(meta_row);
    lv_obj_set_style_pad_column(meta_row, 8, 0);
    lv_obj_set_style_pad_row(meta_row, 8, 0);
    lv_obj_set_flex_flow(
        meta_row, _narrow_layout ? LV_FLEX_FLOW_ROW_WRAP : LV_FLEX_FLOW_ROW
    );
    lv_obj_set_flex_align(
        meta_row, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY
    );
    lv_obj_remove_flag(meta_row, LV_OBJ_FLAG_SCROLLABLE);

    createTxIdTextarea(meta_row, _narrow_layout ? LV_PCT(100) : 148, 64);

    const char *format_names[] = {"STD", "EXT"};
    for (int i = 0; i < 2; i++) {
        _tx_format_buttons[i] = make_button(
            meta_row, format_names[i], _narrow_layout ? 88 : 82, 60
        );
        lv_obj_add_event_cb(
            _tx_format_buttons[i],
            frame_format_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    const char *type_names[] = {"DATA", "RTR"};
    for (int i = 0; i < 2; i++) {
        _tx_type_buttons[i] = make_button(
            meta_row, type_names[i], _narrow_layout ? 96 : 86, 60
        );
        lv_obj_add_event_cb(
            _tx_type_buttons[i],
            frame_type_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    createDlcBox(meta_row, _narrow_layout ? 108 : 98, 60);

    lv_obj_t *data_row = lv_obj_create(card);
    lv_obj_set_size(data_row, LV_PCT(100), 76);
    style_transparent(data_row);
    lv_obj_set_style_pad_column(data_row, 12, 0);
    lv_obj_set_flex_flow(data_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        data_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(data_row, LV_OBJ_FLAG_SCROLLABLE);

    createTxDataTextarea(data_row, 68);

    _send_button = make_button(
        data_row, "Send", _narrow_layout ? 112 : 132, 68, true
    );
    lv_obj_add_event_cb(
        _send_button, send_btn_event_cb, LV_EVENT_CLICKED, this
    );

    make_label(
        card,
        "Classic CAN: maximum 8 data bytes",
        &lv_font_montserrat_20,
        COLOR_TEXT_MUTED
    );
}

void CanTool::createTxIdTextarea(lv_obj_t *parent, int width, int height)
{
    _tx_id_textarea = lv_textarea_create(parent);
    lv_obj_set_size(_tx_id_textarea, width, height);
    lv_textarea_set_one_line(_tx_id_textarea, true);
    lv_textarea_set_max_length(_tx_id_textarea, 10);
    lv_textarea_set_accepted_chars(_tx_id_textarea, "0123456789abcdefABCDEFxX");
    lv_textarea_set_placeholder_text(_tx_id_textarea, "ID (hex)");
    lv_textarea_set_text(_tx_id_textarea, TX_DEFAULT_ID);
    style_textarea(_tx_id_textarea);
    lv_obj_add_event_cb(
        _tx_id_textarea, tx_textarea_event_cb, LV_EVENT_ALL, this
    );
}

void CanTool::createTxDataTextarea(lv_obj_t *parent, int height)
{
    _tx_data_textarea = lv_textarea_create(parent);
    lv_obj_set_width(_tx_data_textarea, 0);
    lv_obj_set_height(_tx_data_textarea, height);
    lv_obj_set_flex_grow(_tx_data_textarea, 1);
    lv_textarea_set_one_line(_tx_data_textarea, true);
    lv_textarea_set_max_length(_tx_data_textarea, 32);
    lv_textarea_set_accepted_chars(
        _tx_data_textarea, "0123456789abcdefABCDEF ,"
    );
    lv_textarea_set_placeholder_text(
        _tx_data_textarea, "Data bytes: 00 11 22 33"
    );
    lv_textarea_set_text(_tx_data_textarea, TX_DEFAULT_DATA);
    style_textarea(_tx_data_textarea);
    lv_obj_add_event_cb(
        _tx_data_textarea, tx_textarea_event_cb, LV_EVENT_ALL, this
    );
}

void CanTool::createDlcBox(lv_obj_t *parent, int width, int height)
{
    lv_obj_t *dlc_box = lv_obj_create(parent);
    lv_obj_set_size(dlc_box, width, height);
    lv_obj_set_style_bg_color(dlc_box, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(dlc_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dlc_box, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(dlc_box, 1, 0);
    lv_obj_set_style_radius(dlc_box, 12, 0);
    lv_obj_set_style_pad_all(dlc_box, 0, 0);
    lv_obj_remove_flag(dlc_box, LV_OBJ_FLAG_SCROLLABLE);
    _dlc_label = make_label(
        dlc_box, "DLC 0", &lv_font_montserrat_20, COLOR_TEXT_MUTED
    );
    lv_obj_center(_dlc_label);
}

void CanTool::createLandscapeUi(lv_obj_t *parent)
{
    const int btn_h = 46;

    // ---- Top bar: title + status | operating mode | bitrate | settings ----
    _top_bar = make_box(parent, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_size(_top_bar, LV_PCT(100), 60);
    lv_obj_set_flex_align(
        _top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t *title = make_box(_top_bar, LV_FLEX_FLOW_ROW, 10);
    make_label(title, "CAN Monitor", &lv_font_montserrat_24, COLOR_TEXT);
    _status_dot = lv_obj_create(title);
    lv_obj_set_size(_status_dot, 12, 12);
    lv_obj_set_style_radius(_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_status_dot, 0, 0);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(COLOR_WARNING), 0);
    lv_obj_set_style_margin_left(_status_dot, 8, 0);
    lv_obj_remove_flag(_status_dot, LV_OBJ_FLAG_SCROLLABLE);
    _status_label = make_label(
        title, "Pins NC", &lv_font_montserrat_20, COLOR_WARNING
    );

    lv_obj_t *mode_row = make_box(_top_bar, LV_FLEX_FLOW_ROW, 4);
    lv_obj_set_height(mode_row, btn_h + 10);
    lv_obj_set_style_bg_color(mode_row, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mode_row, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(mode_row, 1, 0);
    lv_obj_set_style_radius(mode_row, 14, 0);
    lv_obj_set_style_pad_all(mode_row, 4, 0);
    const char *mode_names[] = {"Normal", "Listen Only"};
    for (int i = 0; i < 2; i++) {
        _operating_mode_buttons[i] = make_button(
            mode_row, mode_names[i], i == 0 ? 120 : 160, btn_h
        );
        lv_obj_add_event_cb(
            _operating_mode_buttons[i],
            operating_mode_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    lv_obj_t *summary = make_box(_top_bar, LV_FLEX_FLOW_ROW, 10);
    _bitrate_summary_label = make_label(
        summary, "", &lv_font_montserrat_20, COLOR_TEXT
    );
    make_label(summary, "|", &lv_font_montserrat_20, COLOR_BORDER);
    make_label(summary, "Classic CAN", &lv_font_montserrat_20, COLOR_TEXT_MUTED);

    lv_obj_t *settings_btn = make_button(
        _top_bar, LV_SYMBOL_SETTINGS, 60, btn_h + 6
    );
    lv_obj_add_event_cb(
        settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, this
    );

    // ---- Middle: frame table (left) + bus status and view tools (right) ----
    const int side_w = 270;
    const int gap = 10;
    lv_obj_t *middle = make_box(parent, LV_FLEX_FLOW_ROW, gap);
    lv_obj_set_size(middle, LV_PCT(100), 0);
    lv_obj_set_flex_grow(middle, 1);

    lv_obj_t *frames = lv_obj_create(middle);
    lv_obj_set_size(frames, 0, LV_PCT(100));
    lv_obj_set_flex_grow(frames, 1);
    style_card(frames, 16);
    lv_obj_set_style_pad_all(frames, 12, 0);
    lv_obj_set_style_pad_row(frames, 8, 0);
    lv_obj_set_flex_flow(frames, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(frames, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *heading = make_box(frames, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_size(heading, LV_PCT(100), 30);
    lv_obj_set_flex_align(
        heading, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    make_label(heading, "FRAMES", &lv_font_montserrat_24, COLOR_PRIMARY);
    _frame_count_label = make_label(
        heading, "0 frames", &lv_font_montserrat_20, COLOR_TEXT_MUTED
    );

    createFrameTable(frames, _content_w - side_w - gap - 2 * 13);

    // Scrollable as a fallback: it does not fit while the keyboard is up.
    lv_obj_t *side = lv_obj_create(middle);
    lv_obj_set_size(side, side_w, LV_PCT(100));
    style_card(side, 16);
    lv_obj_set_style_pad_all(side, 14, 0);
    lv_obj_set_style_pad_row(side, 8, 0);
    lv_obj_set_flex_flow(side, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(side, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(side, LV_SCROLLBAR_MODE_AUTO);

    auto add_row = [&](int height) -> lv_obj_t * {
        lv_obj_t *row = make_box(side, LV_FLEX_FLOW_ROW, 8);
        lv_obj_set_size(row, LV_PCT(100), height);
        return row;
    };
    auto add_status = [&](lv_obj_t *row, const char *name, const char *value,
                          uint32_t color, lv_obj_t **value_label) {
        lv_obj_t *cell = make_box(row, LV_FLEX_FLOW_ROW, 8);
        lv_obj_set_width(cell, 0);
        lv_obj_set_flex_grow(cell, 1);
        make_label(cell, name, &lv_font_montserrat_20, COLOR_TEXT_MUTED);
        *value_label = make_label(cell, value, &lv_font_montserrat_22, color);
    };
    auto add_cell = [&](lv_obj_t *row, const char *text,
                        lv_event_cb_t cb) -> lv_obj_t * {
        lv_obj_t *button = make_button(row, text, 0, btn_h);
        lv_obj_set_flex_grow(button, 1);
        lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, this);
        return button;
    };

    lv_obj_t *row = add_row(LV_SIZE_CONTENT);
    add_status(row, "STATE", "Stopped", COLOR_WARNING, &_bus_state_label);
    row = add_row(LV_SIZE_CONTENT);
    add_status(row, "TX", "0", COLOR_TX, &_tx_count_label);
    add_status(row, "RX", "0", COLOR_RX, &_rx_count_label);
    row = add_row(LV_SIZE_CONTENT);
    add_status(row, "TEC", "0", COLOR_TEXT, &_tec_label);
    add_status(row, "REC", "0", COLOR_TEXT, &_rec_label);

    lv_obj_t *show_label = make_label(
        side, "Show", &lv_font_montserrat_20, COLOR_TEXT_MUTED
    );
    lv_obj_set_style_margin_top(show_label, 8, 0);
    row = add_row(btn_h);
    const char *filter_names[] = {"All", "STD", "EXT"};
    for (int i = 0; i < 3; i++) {
        _frame_filter_buttons[i] = add_cell(
            row, filter_names[i], frame_filter_btn_event_cb
        );
    }

    row = add_row(btn_h);
    _advanced_filter_button = add_cell(row, "Filter", advanced_filter_btn_event_cb);
    _advanced_filter_label = lv_obj_get_child(_advanced_filter_button, 0);
    _freeze_button = add_cell(row, "Freeze", freeze_btn_event_cb);
    _freeze_label = lv_obj_get_child(_freeze_button, 0);

    lv_obj_t *clear_button = make_button(side, "Clear", LV_PCT(100), btn_h);
    lv_obj_add_event_cb(
        clear_button, clear_btn_event_cb, LV_EVENT_CLICKED, this
    );

    // ---- Bottom bar: the whole transmit line ----
    const int tx_h = 64;
    lv_obj_t *bottom = make_box(parent, LV_FLEX_FLOW_ROW, 8);
    lv_obj_set_size(bottom, LV_PCT(100), tx_h);

    createTxIdTextarea(bottom, 120, tx_h);

    const char *format_names[] = {"STD", "EXT"};
    for (int i = 0; i < 2; i++) {
        _tx_format_buttons[i] = make_button(bottom, format_names[i], 68, tx_h - 6);
        lv_obj_add_event_cb(
            _tx_format_buttons[i],
            frame_format_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }
    const char *type_names[] = {"DATA", "RTR"};
    for (int i = 0; i < 2; i++) {
        _tx_type_buttons[i] = make_button(
            bottom, type_names[i], i == 0 ? 80 : 68, tx_h - 6
        );
        lv_obj_add_event_cb(
            _tx_type_buttons[i],
            frame_type_btn_event_cb,
            LV_EVENT_CLICKED,
            this
        );
    }

    createDlcBox(bottom, 84, tx_h - 6);
    createTxDataTextarea(bottom, tx_h);

    _send_button = make_button(bottom, "Send", 110, tx_h, true);
    lv_obj_add_event_cb(
        _send_button, send_btn_event_cb, LV_EVENT_CLICKED, this
    );
}

void CanTool::createFilterOverlay(lv_obj_t *parent)
{
    _filter_overlay = lv_obj_create(parent);
    lv_obj_set_size(_filter_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(_filter_overlay);
    lv_obj_set_style_bg_color(_filter_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_filter_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(_filter_overlay, 0, 0);
    lv_obj_set_style_pad_all(_filter_overlay, 0, 0);
    lv_obj_add_flag(_filter_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(_filter_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(_filter_overlay);
    if (_landscape) {
        // Both fields on one line keeps the whole panel above the keyboard.
        lv_obj_set_size(panel, std::min(_content_w, 760), LV_SIZE_CONTENT);
    } else {
        lv_obj_set_size(panel, std::min(_content_w, 620), 430);
    }
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, _landscape ? _root_pad : 24);
    style_card(panel, 22);
    lv_obj_set_style_pad_all(panel, 24, 0);
    lv_obj_set_style_pad_row(panel, 14, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_row = lv_obj_create(panel);
    lv_obj_set_size(title_row, LV_PCT(100), 56);
    style_transparent(title_row);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        title_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);
    make_label(title_row, "Frame Filter", &lv_font_montserrat_30, COLOR_TEXT);
    lv_obj_t *close_btn = make_button(title_row, LV_SYMBOL_CLOSE, 54, 54);
    lv_obj_add_event_cb(
        close_btn, filter_close_btn_event_cb, LV_EVENT_CLICKED, this
    );

    lv_obj_t *fields = panel;
    if (_landscape) {
        fields = make_box(panel, LV_FLEX_FLOW_ROW, 20);
        lv_obj_set_size(fields, LV_PCT(100), 70);
    }

    auto create_filter_field = [&](const char *name, const char *placeholder) {
        lv_obj_t *row = lv_obj_create(fields);
        if (_landscape) {
            lv_obj_set_size(row, 0, 70);
            lv_obj_set_flex_grow(row, 1);
        } else {
            lv_obj_set_size(row, LV_PCT(100), 70);
        }
        style_transparent(row);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            row, LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
        );
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *name_label = make_label(
            row, name, &lv_font_montserrat_22, COLOR_TEXT_MUTED
        );
        lv_obj_set_width(name_label, _landscape ? 90 : 120);

        lv_obj_t *textarea = lv_textarea_create(row);
        lv_obj_set_width(textarea, 0);
        lv_obj_set_height(textarea, 64);
        lv_obj_set_flex_grow(textarea, 1);
        lv_textarea_set_one_line(textarea, true);
        lv_textarea_set_max_length(textarea, 10);
        lv_textarea_set_accepted_chars(
            textarea, "0123456789abcdefABCDEFxX"
        );
        lv_textarea_set_placeholder_text(textarea, placeholder);
        style_textarea(textarea);
        lv_obj_add_event_cb(
            textarea, tx_textarea_event_cb, LV_EVENT_ALL, this
        );
        return textarea;
    };

    _filter_id_textarea = create_filter_field("CAN ID", "0x123");
    _filter_mask_textarea = create_filter_field("Mask", "0x1FFFFFFF");

    _filter_hint_label = make_label(
        panel,
        "Match: (frame ID & mask) == (filter ID & mask)",
        &lv_font_montserrat_16,
        COLOR_TEXT_MUTED
    );
    lv_obj_set_width(_filter_hint_label, LV_PCT(100));
    lv_label_set_long_mode(_filter_hint_label, LV_LABEL_LONG_MODE_WRAP);

    lv_obj_t *actions = lv_obj_create(panel);
    lv_obj_set_size(actions, LV_PCT(100), 64);
    style_transparent(actions);
    lv_obj_set_style_pad_column(actions, 12, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        actions, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *reset_btn = make_button(actions, "Reset", 0, 62);
    lv_obj_set_flex_grow(reset_btn, 1);
    lv_obj_add_event_cb(
        reset_btn, filter_reset_btn_event_cb, LV_EVENT_CLICKED, this
    );
    lv_obj_t *apply_btn = make_button(actions, "Apply", 0, 62, true);
    lv_obj_set_flex_grow(apply_btn, 1);
    lv_obj_add_event_cb(
        apply_btn, filter_apply_btn_event_cb, LV_EVENT_CLICKED, this
    );
}

void CanTool::createSettingsOverlay(lv_obj_t *parent)
{
    _settings_overlay = lv_obj_create(parent);
    lv_obj_set_size(_settings_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(_settings_overlay);
    lv_obj_set_style_bg_color(_settings_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_settings_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(_settings_overlay, 0, 0);
    lv_obj_set_style_pad_all(_settings_overlay, 0, 0);
    lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(_settings_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(_settings_overlay);
    lv_obj_set_size(panel, std::min(_content_w, 620), 430);
    lv_obj_center(panel);
    style_card(panel, 22);
    lv_obj_set_style_pad_all(panel, 24, 0);
    lv_obj_set_style_pad_row(panel, 16, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_row = lv_obj_create(panel);
    lv_obj_set_size(title_row, LV_PCT(100), 58);
    style_transparent(title_row);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        title_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);
    make_label(title_row, "CAN Settings", &lv_font_montserrat_30, COLOR_TEXT);
    lv_obj_t *close_btn = make_button(title_row, LV_SYMBOL_CLOSE, 54, 54);
    lv_obj_add_event_cb(
        close_btn, settings_close_btn_event_cb, LV_EVENT_CLICKED, this
    );

    lv_obj_t *bitrate_row = lv_obj_create(panel);
    lv_obj_set_size(bitrate_row, LV_PCT(100), 78);
    style_transparent(bitrate_row);
    lv_obj_set_flex_flow(bitrate_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        bitrate_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(bitrate_row, LV_OBJ_FLAG_SCROLLABLE);
    make_label(bitrate_row, "Bitrate", &lv_font_montserrat_22, COLOR_TEXT_MUTED);

    _bitrate_dropdown = lv_dropdown_create(bitrate_row);
    lv_obj_set_size(_bitrate_dropdown, 250, 62);
    lv_dropdown_set_options(_bitrate_dropdown, BITRATE_OPTIONS);
    lv_obj_set_style_bg_color(_bitrate_dropdown, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_border_color(_bitrate_dropdown, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(_bitrate_dropdown, 1, 0);
    lv_obj_set_style_radius(_bitrate_dropdown, 12, 0);
    lv_obj_set_style_text_color(_bitrate_dropdown, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(_bitrate_dropdown, &lv_font_montserrat_22, 0);

    lv_obj_t *pin_info = lv_obj_create(panel);
    lv_obj_set_size(pin_info, LV_PCT(100), 106);
    lv_obj_set_style_bg_color(pin_info, lv_color_hex(COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(pin_info, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(pin_info, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(pin_info, 1, 0);
    lv_obj_set_style_radius(pin_info, 12, 0);
    lv_obj_set_style_pad_all(pin_info, 14, 0);
    lv_obj_set_style_pad_row(pin_info, 8, 0);
    lv_obj_set_flex_flow(pin_info, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(pin_info, LV_OBJ_FLAG_SCROLLABLE);
    make_label(
        pin_info, "Fixed board routing", &lv_font_montserrat_20, COLOR_TEXT_MUTED
    );
    char pin_text[48];
    const char *tx_text = CAN_TOOL_TX_GPIO == GPIO_NUM_NC ? "NC" : nullptr;
    const char *rx_text = CAN_TOOL_RX_GPIO == GPIO_NUM_NC ? "NC" : nullptr;
    char tx_gpio_text[12];
    char rx_gpio_text[12];
    if (tx_text == nullptr) {
        snprintf(
            tx_gpio_text, sizeof(tx_gpio_text), "GPIO%d",
            static_cast<int>(CAN_TOOL_TX_GPIO)
        );
        tx_text = tx_gpio_text;
    }
    if (rx_text == nullptr) {
        snprintf(
            rx_gpio_text, sizeof(rx_gpio_text), "GPIO%d",
            static_cast<int>(CAN_TOOL_RX_GPIO)
        );
        rx_text = rx_gpio_text;
    }
    snprintf(pin_text, sizeof(pin_text), "TX  %s       RX  %s", tx_text, rx_text);
    make_label(
        pin_info, pin_text, &lv_font_montserrat_24,
        can_tool_pins_ready() ? COLOR_SUCCESS : COLOR_WARNING
    );

    lv_obj_t *apply_btn = make_button(panel, "Apply", LV_PCT(100), 64, true);
    lv_obj_add_event_cb(
        apply_btn, apply_btn_event_cb, LV_EVENT_CLICKED, this
    );
}

void CanTool::createKeyboard()
{
    if (_screen == nullptr || _keyboard != nullptr) {
        return;
    }
    _keyboard = lv_keyboard_create(_screen);
    lv_keyboard_set_map(
        _keyboard, LV_KEYBOARD_MODE_USER_1, hex_kb_map, hex_kb_ctrl
    );
    lv_keyboard_set_mode(_keyboard, LV_KEYBOARD_MODE_USER_1);
    if (_landscape) {
        lv_obj_set_size(_keyboard, LV_PCT(100), _keyboard_h);
    } else {
        lv_obj_set_size(_keyboard, LV_PCT(100), LV_PCT(38));
    }
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(COLOR_CARD), 0);
    lv_obj_set_style_border_color(_keyboard, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(_keyboard, 1, 0);
    lv_obj_set_style_text_font(_keyboard, &lv_font_montserrat_24, 0);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void CanTool::loadConfigToWidgets()
{
    if (_bitrate_dropdown != nullptr) {
        lv_dropdown_set_selected(
            _bitrate_dropdown, bitrate_to_index(_config.bitrate)
        );
    }
}

void CanTool::applyConfig()
{
    if (_bitrate_dropdown == nullptr) {
        return;
    }

    CanToolConfig next = _config;
    const uint32_t selected = lv_dropdown_get_selected(_bitrate_dropdown);
    if (selected >= BITRATE_OPTION_COUNT) {
        setStatus("Bad bitrate", COLOR_ERROR);
        return;
    }
    next.bitrate = bitrate_values[selected];
    if (!commitConfig(next, true)) {
        loadConfigToWidgets();
        return;
    }
    lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
}

void CanTool::updateConfigWidgets()
{
    for (int i = 0; i < 2; i++) {
        set_checked(
            _operating_mode_buttons[i],
            static_cast<int>(_config.mode) == i
        );
        set_checked(_tx_format_buttons[i], _tx_extended == (i == 1));
        set_checked(_tx_type_buttons[i], _tx_remote == (i == 1));
    }
    for (int i = 0; i < 3; i++) {
        set_checked(
            _frame_filter_buttons[i],
            static_cast<int>(_config.frame_filter) == i
        );
    }
    set_checked(_advanced_filter_button, _advanced_filter_enabled);
    if (_advanced_filter_label != nullptr) {
        set_label_text_if_changed(
            _advanced_filter_label,
            _advanced_filter_enabled ? "Filter ON" : "Filter"
        );
    }

    set_checked(_freeze_button, _frames_frozen);
    if (_freeze_label != nullptr) {
        lv_label_set_text(_freeze_label, _frames_frozen ? "Resume" : "Freeze");
    }

    if (_tx_data_textarea != nullptr) {
        if (_tx_remote) {
            lv_obj_add_state(_tx_data_textarea, LV_STATE_DISABLED);
        } else {
            lv_obj_remove_state(_tx_data_textarea, LV_STATE_DISABLED);
        }
    }
    if (_send_button != nullptr) {
        const bool can_send =
            _config.mode != CanOperatingMode::ListenOnly &&
            _service_status.running &&
            !_service_status.listen_only &&
            _service_status.state != CanToolBusState::BusOff &&
            _service_status.state != CanToolBusState::Stopped;
        if (!can_send) {
            lv_obj_add_state(_send_button, LV_STATE_DISABLED);
        } else {
            lv_obj_remove_state(_send_button, LV_STATE_DISABLED);
        }
    }
}

void CanTool::updateConnectionStatus()
{
    const bool save_failed = _config_writer.takeFailure();
    const CanToolServiceStatus status = _service.getStatus();
    const bool tx_failed =
        status.tx_failed_count > _service_status.tx_failed_count;
    const bool frame_summary_changed =
        status.tx_count != _service_status.tx_count ||
        status.rx_count != _service_status.rx_count ||
        status.dropped_count != _service_status.dropped_count ||
        status.tx_failed_count != _service_status.tx_failed_count;
    _service_status = status;
    if (frame_summary_changed) {
        _frame_render_dirty = true;
    }
    const char *state_text = "Stopped";
    uint32_t state_color = COLOR_WARNING;

    if (!can_tool_pins_ready()) {
        setStatus("Pins NC", COLOR_WARNING);
    } else if (!status.running) {
        setStatus(
            status.last_error == ESP_OK ? "Stopped" : "CAN start error",
            status.last_error == ESP_OK ? COLOR_WARNING : COLOR_ERROR
        );
    } else {
        switch (status.state) {
        case CanToolBusState::Active:
            setStatus(status.listen_only ? "Listening" : "Running", COLOR_SUCCESS);
            state_text = status.listen_only ? "Listen" : "Active";
            state_color = COLOR_SUCCESS;
            break;
        case CanToolBusState::Warning:
            setStatus("Bus warning", COLOR_WARNING);
            state_text = "Warning";
            state_color = COLOR_WARNING;
            break;
        case CanToolBusState::Passive:
            setStatus("Error passive", COLOR_ERROR);
            state_text = "Passive";
            state_color = COLOR_ERROR;
            break;
        case CanToolBusState::BusOff:
            setStatus("Bus off", COLOR_ERROR);
            state_text = "Bus Off";
            state_color = COLOR_ERROR;
            break;
        case CanToolBusState::Stopped:
        default:
            setStatus("Stopped", COLOR_WARNING);
            break;
        }
    }
    if (tx_failed) {
        setStatus("TX failed", COLOR_ERROR);
    }
    if (save_failed) {
        setStatus("Save failed", COLOR_ERROR);
    }
    if (_send_button != nullptr) {
        const bool can_send =
            status.running &&
            !status.listen_only &&
            status.state != CanToolBusState::BusOff &&
            status.state != CanToolBusState::Stopped;
        if (can_send) {
            lv_obj_remove_state(_send_button, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(_send_button, LV_STATE_DISABLED);
        }
    }

    if (_bus_state_label != nullptr) {
        set_label_text_if_changed(_bus_state_label, state_text);
        lv_obj_set_style_text_color(
            _bus_state_label, lv_color_hex(state_color), 0
        );
    }
    if (_tx_count_label != nullptr && _rx_count_label != nullptr &&
        _tec_label != nullptr && _rec_label != nullptr) {
        snprintf(
            _tx_count_text, sizeof(_tx_count_text), "%llu",
            static_cast<unsigned long long>(status.tx_count)
        );
        lv_label_set_text_static(_tx_count_label, _tx_count_text);
        snprintf(
            _rx_count_text, sizeof(_rx_count_text), "%llu",
            static_cast<unsigned long long>(status.rx_count)
        );
        lv_label_set_text_static(_rx_count_label, _rx_count_text);
        snprintf(
            _tec_text, sizeof(_tec_text), "%u",
            static_cast<unsigned>(status.tx_error_count)
        );
        lv_label_set_text_static(_tec_label, _tec_text);
        snprintf(
            _rec_text, sizeof(_rec_text), "%u",
            static_cast<unsigned>(status.rx_error_count)
        );
        lv_label_set_text_static(_rec_label, _rec_text);
    }
}

void CanTool::updateBitrateSummary()
{
    if (_bitrate_summary_label == nullptr) {
        return;
    }

    char text[24];
    if (_config.bitrate == 1000000) {
        snprintf(text, sizeof(text), "1 Mbit/s");
    } else {
        snprintf(text, sizeof(text), "%d kbit/s", _config.bitrate / 1000);
    }
    lv_label_set_text(_bitrate_summary_label, text);
}

void CanTool::updateDlcLabel()
{
    if (_dlc_label == nullptr) {
        return;
    }

    const int length = _tx_remote ? 0 :
                       parse_can_data(lv_textarea_get_text(_tx_data_textarea));
    if (length < 0) {
        lv_label_set_text(_dlc_label, "DLC --");
        lv_obj_set_style_text_color(_dlc_label, lv_color_hex(COLOR_ERROR), 0);
        return;
    }

    char text[16];
    snprintf(text, sizeof(text), "DLC %d", length);
    lv_label_set_text(_dlc_label, text);
    lv_obj_set_style_text_color(_dlc_label, lv_color_hex(COLOR_TEXT_MUTED), 0);
}

void CanTool::setStatus(const char *text, uint32_t color)
{
    if (_status_label == nullptr || _status_dot == nullptr) {
        return;
    }
    set_label_text_if_changed(_status_label, text);
    lv_obj_set_style_text_color(_status_label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(_status_dot, lv_color_hex(color), 0);
}

void CanTool::validateAndQueueFrame()
{
    if (_config.mode == CanOperatingMode::ListenOnly) {
        setStatus("Listen only", COLOR_WARNING);
        return;
    }
    uint32_t id = 0;
    if (!parse_can_id(
            lv_textarea_get_text(_tx_id_textarea), _tx_extended, &id
        )) {
        setStatus(_tx_extended ? "Bad EXT ID" : "Bad STD ID", COLOR_ERROR);
        return;
    }

    uint8_t data[8] = {};
    int length = 0;
    if (!_tx_remote) {
        length = parse_can_data(lv_textarea_get_text(_tx_data_textarea), data);
        if (length == -2) {
            setStatus("> 8 bytes", COLOR_ERROR);
            return;
        }
        if (length < 0) {
            setStatus("Bad data", COLOR_ERROR);
            return;
        }
    }

    if (!can_tool_pins_ready()) {
        setStatus("Pins NC", COLOR_WARNING);
        return;
    }

    const esp_err_t err = _service.send(
        id,
        _tx_extended,
        _tx_remote,
        data,
        static_cast<uint8_t>(length)
    );
    if (err == ESP_OK) {
        setStatus("TX queued", COLOR_TX);
    } else if (err == ESP_ERR_TIMEOUT) {
        setStatus("TX queue full", COLOR_WARNING);
    } else {
        setStatus("TX failed", COLOR_ERROR);
    }
}

bool CanTool::restartService()
{
    esp_err_t err = ESP_OK;
    if (!can_tool_pins_ready()) {
        err = _service.stop();
    } else {
        err = _service.start(
            _config.bitrate,
            _config.mode == CanOperatingMode::ListenOnly
        );
    }
    updateConfigWidgets();
    updateConnectionStatus();
    return err == ESP_OK;
}

bool CanTool::commitConfig(
    const CanToolConfig &config, bool restart_driver
)
{
    const CanToolConfig previous = _config;
    const bool config_changed =
        config.mode != previous.mode ||
        config.bitrate != previous.bitrate ||
        config.frame_filter != previous.frame_filter;
    if (restart_driver) {
        esp_err_t err = ESP_OK;
        if (can_tool_pins_ready()) {
            err = _service.start(
                config.bitrate,
                config.mode == CanOperatingMode::ListenOnly
            );
        } else {
            err = _service.stop();
        }
        if (err != ESP_OK) {
            bool restored = !can_tool_pins_ready();
            if (can_tool_pins_ready()) {
                restored = _service.start(
                    previous.bitrate,
                    previous.mode == CanOperatingMode::ListenOnly
                ) == ESP_OK;
            }
            clearFrameHistory();
            updateConfigWidgets();
            updateConnectionStatus();
            renderFrames();
            setStatus(restored ? "Apply failed" : "CAN error", COLOR_ERROR);
            return false;
        }
        clearFrameHistory();
    }

    _config = config;
    updateBitrateSummary();
    updateConfigWidgets();
    updateConnectionStatus();
    renderFrames();
    if (config_changed && !requestConfigSave()) {
        setStatus("Save queue failed", COLOR_ERROR);
    }
    return true;
}

bool CanTool::requestConfigSave()
{
    return _config_writer.requestSave(_config);
}

void CanTool::drainServiceFrames(size_t budget)
{
    CanToolFrame chunk[UI_DRAIN_CHUNK];
    size_t drained = 0;
    bool changed = false;
    while (drained < budget) {
        const size_t request_count = std::min(
            UI_DRAIN_CHUNK, budget - drained
        );
        const size_t count = _service.readFrames(chunk, request_count);
        if (count == 0) {
            break;
        }
        for (size_t i = 0; i < count; i++) {
            appendFrame(chunk[i]);
        }
        drained += count;
        changed = true;
        if (count < request_count) {
            break;
        }
    }
    if (changed) {
        _frame_render_dirty = true;
    }
}

void CanTool::appendFrame(const CanToolFrame &frame)
{
    _frame_history[_frame_history_head] = frame;
    _frame_history_head = (_frame_history_head + 1) % FRAME_HISTORY_SIZE;
    if (_frame_history_size < FRAME_HISTORY_SIZE) {
        _frame_history_size++;
    }
}

void CanTool::clearFrameHistory()
{
    _frame_history_head = 0;
    _frame_history_size = 0;
    _frame_time_text[0] = '\0';
    _frame_direction_text[0] = '\0';
    _frame_format_text[0] = '\0';
    _frame_type_text[0] = '\0';
    _frame_id_text[0] = '\0';
    _frame_length_text[0] = '\0';
    _frame_data_text[0] = '\0';
}

void CanTool::renderFrames()
{
    if (_frame_count_label == nullptr || _frames_empty_label == nullptr) {
        return;
    }
    for (lv_obj_t *label : _frame_column_labels) {
        if (label == nullptr) {
            return;
        }
    }
    _frame_render_dirty = false;

    const CanToolServiceStatus &status = _service_status;
    const unsigned long long total = static_cast<unsigned long long>(
        status.rx_count + status.tx_count
    );
    if (status.dropped_count > 0 || status.tx_failed_count > 0) {
        snprintf(
            _frame_count_text,
            sizeof(_frame_count_text),
            "%llu frames | D:%llu F:%llu",
            total,
            static_cast<unsigned long long>(status.dropped_count),
            static_cast<unsigned long long>(status.tx_failed_count)
        );
    } else {
        snprintf(
            _frame_count_text, sizeof(_frame_count_text), "%llu frames", total
        );
    }
    lv_label_set_text_static(_frame_count_label, _frame_count_text);

    struct ColumnBuffer {
        char *text;
        size_t capacity;
        size_t used;
    };
    ColumnBuffer columns[FRAME_COLUMN_COUNT] = {
        {_frame_time_text, sizeof(_frame_time_text), 0},
        {_frame_direction_text, sizeof(_frame_direction_text), 0},
        {_frame_format_text, sizeof(_frame_format_text), 0},
        {_frame_type_text, sizeof(_frame_type_text), 0},
        {_frame_id_text, sizeof(_frame_id_text), 0},
        {_frame_length_text, sizeof(_frame_length_text), 0},
        {_frame_data_text, sizeof(_frame_data_text), 0},
    };
    for (auto &column : columns) {
        column.text[0] = '\0';
    }

    size_t rendered = 0;
    size_t matched = 0;

    for (size_t offset = 0; offset < _frame_history_size; offset++) {
        const size_t index = (
            _frame_history_head + FRAME_HISTORY_SIZE - 1 - offset
        ) % FRAME_HISTORY_SIZE;
        const CanToolFrame &frame = _frame_history[index];
        const bool format_accepted =
            _config.frame_filter == CanFrameFilter::All ||
            (_config.frame_filter == CanFrameFilter::Standard && !frame.extended) ||
            (_config.frame_filter == CanFrameFilter::Extended && frame.extended);
        const bool id_accepted =
            !_advanced_filter_enabled ||
            can_id_matches_filter(
                frame.id, _advanced_filter_id, _advanced_filter_mask
            );
        if (!format_accepted || !id_accepted) {
            continue;
        }
        matched++;
        if (rendered >= FRAME_RENDER_MAX_LINES) {
            continue;
        }

        char time_text[24] = {};
        char id_text[16] = {};
        char length_text[8] = {};
        char data_text[32] = {};
        snprintf(
            time_text,
            sizeof(time_text),
            "%llu",
            static_cast<unsigned long long>(frame.timestamp_us / 1000)
        );
        snprintf(
            id_text,
            sizeof(id_text),
            "0x%08lX",
            static_cast<unsigned long>(frame.id)
        );
        snprintf(
            length_text,
            sizeof(length_text),
            "%u",
            static_cast<unsigned>(frame.dlc)
        );

        if (frame.remote) {
            snprintf(data_text, sizeof(data_text), "--");
        } else {
            size_t data_used = 0;
            for (uint8_t i = 0;
                 i < frame.dlc && data_used < sizeof(data_text) - 1;
                 i++) {
                const int written = snprintf(
                    data_text + data_used,
                    sizeof(data_text) - data_used,
                    i + 1 == frame.dlc ? "%02X" : "%02X ",
                    static_cast<unsigned>(frame.data[i])
                );
                if (written <= 0) {
                    break;
                }
                const size_t remaining = sizeof(data_text) - data_used;
                if (static_cast<size_t>(written) >= remaining) {
                    data_used = sizeof(data_text) - 1;
                    break;
                }
                data_used += static_cast<size_t>(written);
            }
            if (frame.dlc == 0) {
                snprintf(data_text, sizeof(data_text), "--");
            }
        }

        const char *values[FRAME_COLUMN_COUNT] = {
            time_text,
            frame.tx ? "TX" : "RX",
            frame.remote ? "RTR" : "DATA",
            frame.extended ? "EXT" : "STD",
            id_text,
            length_text,
            data_text,
        };
        size_t lengths[FRAME_COLUMN_COUNT] = {};
        bool has_room = true;
        for (size_t i = 0; i < FRAME_COLUMN_COUNT; i++) {
            lengths[i] = std::strlen(values[i]);
            if (lengths[i] + 2 > columns[i].capacity - columns[i].used) {
                has_room = false;
                break;
            }
        }
        if (!has_room) {
            break;
        }

        for (size_t i = 0; i < FRAME_COLUMN_COUNT; i++) {
            std::memcpy(
                columns[i].text + columns[i].used, values[i], lengths[i]
            );
            columns[i].used += lengths[i];
            columns[i].text[columns[i].used++] = '\n';
            columns[i].text[columns[i].used] = '\0';
        }
        rendered++;
    }

    // Each column owns one lifetime-stable text buffer. Updating seven static
    // labels keeps rows aligned without allocating one LVGL object per frame.
    for (size_t i = 0; i < FRAME_COLUMN_COUNT; i++) {
        lv_label_set_text_static(_frame_column_labels[i], columns[i].text);
    }

    if (rendered == 0) {
        for (lv_obj_t *label : _frame_column_labels) {
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_remove_flag(_frames_empty_label, LV_OBJ_FLAG_HIDDEN);
        set_label_text_if_changed(
            _frames_empty_label,
            (_frame_history_size > 0 && matched == 0) ?
            "No matching frames" : "No frames received"
        );
    } else {
        for (lv_obj_t *label : _frame_column_labels) {
            lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(_frames_empty_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void CanTool::hideKeyboard()
{
    if (_keyboard == nullptr) {
        return;
    }
    lv_keyboard_set_textarea(_keyboard, nullptr);
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
    setKeyboardLayout(false);
}

// Landscape only: shrink the layout above the keyboard so the transmit line
// stays visible; the top bar gives its height to the frame table meanwhile.
void CanTool::setKeyboardLayout(bool keyboard_visible)
{
    if (!_landscape || _root == nullptr) {
        return;
    }
    if (_top_bar != nullptr) {
        if (keyboard_visible) {
            lv_obj_add_flag(_top_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(_top_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    const int full_h = _layout_h - 2 * _root_pad;
    lv_obj_set_height(
        _root, keyboard_visible ? (full_h - _keyboard_h + _root_pad - 8) : full_h
    );
}

void CanTool::settings_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->hideKeyboard();
    if (self->_settings_overlay == nullptr) {
        self->createSettingsOverlay(self->_screen);
    }
    self->loadConfigToWidgets();
    lv_obj_remove_flag(self->_settings_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(self->_settings_overlay);
}

void CanTool::settings_close_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    lv_obj_add_flag(self->_settings_overlay, LV_OBJ_FLAG_HIDDEN);
}

void CanTool::apply_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->applyConfig();
}

void CanTool::operating_mode_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    for (int i = 0; i < 2; i++) {
        if (target != self->_operating_mode_buttons[i]) {
            continue;
        }
        if (static_cast<int>(self->_config.mode) == i) {
            return;
        }

        CanToolConfig next = self->_config;
        next.mode = static_cast<CanOperatingMode>(i);
        if (!self->commitConfig(next, true)) {
            self->updateConfigWidgets();
        }
        return;
    }
}

void CanTool::frame_filter_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
    for (int i = 0; i < 3; i++) {
        if (target != self->_frame_filter_buttons[i]) {
            continue;
        }
        if (static_cast<int>(self->_config.frame_filter) == i) {
            return;
        }

        CanToolConfig next = self->_config;
        next.frame_filter = static_cast<CanFrameFilter>(i);
        self->commitConfig(next, false);
        return;
    }
}

void CanTool::advanced_filter_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->hideKeyboard();
    if (self->_filter_overlay == nullptr) {
        self->createFilterOverlay(self->_screen);
    }

    char id_text[16];
    char mask_text[16];
    snprintf(
        id_text, sizeof(id_text), "0x%lX",
        static_cast<unsigned long>(self->_advanced_filter_id)
    );
    snprintf(
        mask_text, sizeof(mask_text), "0x%lX",
        static_cast<unsigned long>(self->_advanced_filter_mask)
    );
    lv_textarea_set_text(self->_filter_id_textarea, id_text);
    lv_textarea_set_text(self->_filter_mask_textarea, mask_text);
    set_label_text_if_changed(
        self->_filter_hint_label,
        "Match: (frame ID & mask) == (filter ID & mask)"
    );
    lv_obj_set_style_text_color(
        self->_filter_hint_label, lv_color_hex(COLOR_TEXT_MUTED), 0
    );
    lv_obj_remove_flag(self->_filter_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(self->_filter_overlay);
}

void CanTool::filter_close_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->hideKeyboard();
    lv_obj_add_flag(self->_filter_overlay, LV_OBJ_FLAG_HIDDEN);
}

void CanTool::filter_reset_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->_advanced_filter_enabled = false;
    self->hideKeyboard();
    lv_obj_add_flag(self->_filter_overlay, LV_OBJ_FLAG_HIDDEN);
    self->updateConfigWidgets();
    self->renderFrames();
    self->setStatus("Filter cleared", COLOR_SUCCESS);
}

void CanTool::filter_apply_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    uint32_t filter_id = 0;
    uint32_t filter_mask = 0;
    if (!parse_can_id(
            lv_textarea_get_text(self->_filter_id_textarea),
            true,
            &filter_id
        )) {
        set_label_text_if_changed(self->_filter_hint_label, "Invalid CAN ID");
        lv_obj_set_style_text_color(
            self->_filter_hint_label, lv_color_hex(COLOR_ERROR), 0
        );
        return;
    }
    if (!parse_can_id(
            lv_textarea_get_text(self->_filter_mask_textarea),
            true,
            &filter_mask
        )) {
        set_label_text_if_changed(self->_filter_hint_label, "Invalid mask");
        lv_obj_set_style_text_color(
            self->_filter_hint_label, lv_color_hex(COLOR_ERROR), 0
        );
        return;
    }

    self->_advanced_filter_id = filter_id;
    self->_advanced_filter_mask = filter_mask;
    self->_advanced_filter_enabled = true;
    self->hideKeyboard();
    lv_obj_add_flag(self->_filter_overlay, LV_OBJ_FLAG_HIDDEN);
    self->updateConfigWidgets();
    self->renderFrames();
    self->setStatus("Filter active", COLOR_SUCCESS);
}

void CanTool::freeze_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->_frames_frozen = !self->_frames_frozen;
    self->updateConfigWidgets();
    if (!self->_frames_frozen) {
        self->renderFrames();
    }
}

void CanTool::clear_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->_service.clear();
    self->clearFrameHistory();
    self->updateConnectionStatus();
    self->renderFrames();
}

void CanTool::frame_format_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->_tx_extended =
        lv_event_get_target(e) == self->_tx_format_buttons[1];
    self->updateConfigWidgets();
}

void CanTool::frame_type_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->_tx_remote =
        lv_event_get_target(e) == self->_tx_type_buttons[1];
    self->hideKeyboard();
    self->updateConfigWidgets();
    self->updateDlcLabel();
}

void CanTool::send_btn_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    self->hideKeyboard();
    self->validateAndQueueFrame();
}

void CanTool::tx_textarea_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));

    // CLICKED re-opens the keyboard on a textarea that is still focused;
    // CANCEL is the keyboard's own close key.
    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        if (self->_keyboard == nullptr) {
            self->createKeyboard();
        }
        lv_keyboard_set_textarea(self->_keyboard, target);
        lv_keyboard_set_mode(self->_keyboard, LV_KEYBOARD_MODE_USER_1);
        lv_obj_remove_flag(self->_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(self->_keyboard);
        lv_obj_align(self->_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        // The filter popup sits above the keyboard on its own.
        self->setKeyboardLayout(
            target == self->_tx_id_textarea || target == self->_tx_data_textarea
        );
    } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY ||
               code == LV_EVENT_CANCEL) {
        self->hideKeyboard();
    } else if (code == LV_EVENT_VALUE_CHANGED &&
               target == self->_tx_data_textarea) {
        self->updateDlcLabel();
    }
}

void CanTool::screen_size_changed_event_cb(lv_event_t *e)
{
    auto *self = static_cast<CanTool *>(lv_event_get_user_data(e));
    if (self == nullptr || self->_screen == nullptr ||
        self->_ui_paused || self->_layout_update_pending) {
        return;
    }

    self->_layout_update_pending = true;
    if (lv_async_call(layout_update_async_cb, self) != LV_RESULT_OK) {
        self->_layout_update_pending = false;
    }
}

void CanTool::layout_update_async_cb(void *user_data)
{
    auto *self = static_cast<CanTool *>(user_data);
    if (self == nullptr) {
        return;
    }
    self->_layout_update_pending = false;
    if (self->_screen == nullptr) {
        return;
    }
    self->rebuildUi(
        lv_obj_get_width(self->_screen),
        lv_obj_get_height(self->_screen)
    );
}

void CanTool::ui_refresh_timer_cb(lv_timer_t *timer)
{
    auto *self = static_cast<CanTool *>(lv_timer_get_user_data(timer));
    if (self == nullptr || self->_ui_paused || self->_screen == nullptr) {
        return;
    }

    self->drainServiceFrames(UI_DRAIN_BUDGET);
    self->_status_poll_divider++;
    if (self->_status_poll_divider >= STATUS_POLL_TICKS) {
        self->_status_poll_divider = 0;
        self->updateConnectionStatus();
    }
    self->_frame_render_divider++;
    if (self->_frame_render_divider >= FRAME_RENDER_TICKS) {
        self->_frame_render_divider = 0;
        if (self->_frame_render_dirty && !self->_frames_frozen) {
            self->renderFrames();
        }
    }
}

bool CanTool::back(void)
{
    if (_keyboard != nullptr &&
        !lv_obj_has_flag(_keyboard, LV_OBJ_FLAG_HIDDEN)) {
        hideKeyboard();
        return true;
    }
    if (_filter_overlay != nullptr &&
        !lv_obj_has_flag(_filter_overlay, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(_filter_overlay, LV_OBJ_FLAG_HIDDEN);
        return true;
    }
    if (_settings_overlay != nullptr &&
        !lv_obj_has_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(_settings_overlay, LV_OBJ_FLAG_HIDDEN);
        return true;
    }
    ESP_UTILS_CHECK_FALSE_RETURN(
        notifyCoreClosed(), false, "Notify core closed failed"
    );
    return true;
}

bool CanTool::close(void)
{
    ESP_UTILS_LOGD("Close(@0x%p)", this);
    _ui_paused = true;
    lv_async_call_cancel(layout_update_async_cb, this);
    _layout_update_pending = false;
    hideKeyboard();
    if (_ui_timer != nullptr) {
        lv_timer_pause(_ui_timer);
    }
    return true;
}

bool CanTool::pause(void)
{
    ESP_UTILS_LOGD("Pause(@0x%p)", this);
    _ui_paused = true;
    lv_async_call_cancel(layout_update_async_cb, this);
    _layout_update_pending = false;
    hideKeyboard();
    if (_ui_timer != nullptr) {
        lv_timer_pause(_ui_timer);
    }
    return true;
}

bool CanTool::resume(void)
{
    ESP_UTILS_LOGD("Resume(@0x%p)", this);
    _ui_paused = false;
    if (_screen != nullptr) {
        rebuildUi(lv_obj_get_width(_screen), lv_obj_get_height(_screen));
    }
    if (_ui_timer != nullptr) {
        lv_timer_resume(_ui_timer);
        lv_timer_ready(_ui_timer);
    }
    return true;
}

bool CanTool::cleanResource(void)
{
    ESP_UTILS_LOGD("CleanResource(@0x%p)", this);
    _ui_paused = true;
    if (_ui_timer != nullptr) {
        lv_timer_delete(_ui_timer);
        _ui_timer = nullptr;
    }
    lv_async_call_cancel(layout_update_async_cb, this);
    _layout_update_pending = false;
    if (_screen != nullptr) {
        lv_obj_remove_event_cb_with_user_data(
            _screen, screen_size_changed_event_cb, this
        );
    }
    _service.stop();
    _config_writer.stop();
    clearFrameHistory();
    clearUiPointers();
    _screen = nullptr;
    _layout_w = 0;
    _layout_h = 0;
    _frame_render_dirty = false;
    _frame_render_divider = 0;
    _status_poll_divider = 0;
    return true;
}

// The app is a singleton owned by itself; the registry must not delete it.
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, CanTool, APP_NAME, []()
{
    return std::shared_ptr<CanTool>(CanTool::requestInstance(), [](CanTool *) {});
})

} // namespace esp_brookesia::apps
