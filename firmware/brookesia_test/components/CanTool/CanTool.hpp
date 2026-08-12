#pragma once

#include <cstddef>
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "can_tool_config.hpp"
#include "can_tool_config_writer.hpp"
#include "can_tool_service.hpp"

namespace esp_brookesia::apps {

class CanTool : public systems::phone::App {
public:
    static CanTool *requestInstance(
        bool use_status_bar = false, bool use_navigation_bar = false
    );
    ~CanTool();

protected:
    CanTool(bool use_status_bar, bool use_navigation_bar);

    bool run(void) override;
    bool back(void) override;
    bool close(void) override;
    bool pause(void) override;
    bool resume(void) override;
    bool cleanResource(void) override;

private:
    void createUi(lv_obj_t *screen, int width, int height);
    void rebuildUi(int width, int height);
    void deleteUiObjects();
    void clearUiPointers();
    void createHeader(lv_obj_t *parent);
    void createConnectionCard(lv_obj_t *parent);
    void createBusStatus(lv_obj_t *parent);
    void createFramesSection(lv_obj_t *parent);
    void createTransmitSection(lv_obj_t *parent);
    void createFilterOverlay(lv_obj_t *parent);
    void createSettingsOverlay(lv_obj_t *parent);
    void createKeyboard();
    void loadConfigToWidgets();
    void applyConfig();
    void updateConfigWidgets();
    void updateConnectionStatus();
    void updateBitrateSummary();
    void updateDlcLabel();
    void setStatus(const char *text, uint32_t color);
    void validateAndQueueFrame();
    void hideKeyboard();
    bool commitConfig(const CanToolConfig &config, bool restart_driver);
    bool restartService();
    bool requestConfigSave();
    void drainServiceFrames(size_t budget);
    void appendFrame(const CanToolFrame &frame);
    void clearFrameHistory();
    void renderFrames();

    static void settings_btn_event_cb(lv_event_t *e);
    static void settings_close_btn_event_cb(lv_event_t *e);
    static void apply_btn_event_cb(lv_event_t *e);
    static void operating_mode_btn_event_cb(lv_event_t *e);
    static void frame_filter_btn_event_cb(lv_event_t *e);
    static void advanced_filter_btn_event_cb(lv_event_t *e);
    static void filter_close_btn_event_cb(lv_event_t *e);
    static void filter_reset_btn_event_cb(lv_event_t *e);
    static void filter_apply_btn_event_cb(lv_event_t *e);
    static void freeze_btn_event_cb(lv_event_t *e);
    static void clear_btn_event_cb(lv_event_t *e);
    static void frame_format_btn_event_cb(lv_event_t *e);
    static void frame_type_btn_event_cb(lv_event_t *e);
    static void send_btn_event_cb(lv_event_t *e);
    static void tx_textarea_event_cb(lv_event_t *e);
    static void screen_size_changed_event_cb(lv_event_t *e);
    static void layout_update_async_cb(void *user_data);
    static void ui_refresh_timer_cb(lv_timer_t *timer);

    CanToolConfig _config;
    CanToolService _service;
    CanToolServiceStatus _service_status;
    CanToolConfigWriter _config_writer;

    int _content_w = 0;
    int _layout_w = 0;
    int _layout_h = 0;
    bool _compact_layout = false;
    bool _narrow_layout = false;
    bool _layout_update_pending = false;
    bool _frames_frozen = false;
    bool _advanced_filter_enabled = false;
    bool _tx_extended = false;
    bool _tx_remote = false;
    bool _ui_paused = false;
    bool _frame_render_dirty = false;
    uint8_t _frame_render_divider = 0;
    uint8_t _status_poll_divider = 0;
    uint32_t _advanced_filter_id = 0x123;
    uint32_t _advanced_filter_mask = 0x1FFFFFFF;

    static constexpr size_t FRAME_HISTORY_SIZE = 64;
    static constexpr size_t FRAME_COLUMN_COUNT = 7;
    CanToolFrame _frame_history[FRAME_HISTORY_SIZE] = {};
    size_t _frame_history_head = 0;
    size_t _frame_history_size = 0;
    char _frame_time_text[512] = {};
    char _frame_direction_text[80] = {};
    char _frame_format_text[128] = {};
    char _frame_type_text[112] = {};
    char _frame_id_text[320] = {};
    char _frame_length_text[64] = {};
    char _frame_data_text[640] = {};
    char _frame_count_text[96] = {};
    char _tx_count_text[24] = {};
    char _rx_count_text[24] = {};
    char _tec_text[16] = {};
    char _rec_text[16] = {};

    lv_obj_t *_screen = nullptr;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_status_dot = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_operating_mode_buttons[2] = {};
    lv_obj_t *_bitrate_summary_label = nullptr;
    lv_obj_t *_bus_state_label = nullptr;
    lv_obj_t *_tx_count_label = nullptr;
    lv_obj_t *_rx_count_label = nullptr;
    lv_obj_t *_tec_label = nullptr;
    lv_obj_t *_rec_label = nullptr;
    lv_obj_t *_frame_filter_buttons[3] = {};
    lv_obj_t *_advanced_filter_button = nullptr;
    lv_obj_t *_advanced_filter_label = nullptr;
    lv_obj_t *_freeze_button = nullptr;
    lv_obj_t *_freeze_label = nullptr;
    lv_obj_t *_frame_count_label = nullptr;
    lv_obj_t *_frame_column_labels[FRAME_COLUMN_COUNT] = {};
    lv_obj_t *_frames_empty_label = nullptr;
    lv_obj_t *_tx_id_textarea = nullptr;
    lv_obj_t *_tx_data_textarea = nullptr;
    lv_obj_t *_tx_format_buttons[2] = {};
    lv_obj_t *_tx_type_buttons[2] = {};
    lv_obj_t *_dlc_label = nullptr;
    lv_obj_t *_send_button = nullptr;
    lv_obj_t *_filter_overlay = nullptr;
    lv_obj_t *_filter_id_textarea = nullptr;
    lv_obj_t *_filter_mask_textarea = nullptr;
    lv_obj_t *_filter_hint_label = nullptr;
    lv_obj_t *_settings_overlay = nullptr;
    lv_obj_t *_bitrate_dropdown = nullptr;
    lv_obj_t *_keyboard = nullptr;
    lv_timer_t *_ui_timer = nullptr;

    static CanTool *_instance;
};

} // namespace esp_brookesia::apps
