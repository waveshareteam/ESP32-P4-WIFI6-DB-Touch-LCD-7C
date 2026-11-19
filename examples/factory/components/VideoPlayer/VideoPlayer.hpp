#pragma once

#include <atomic>

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "board.h"
#include "esp_lcd_mipi_dsi.h"
#include "driver/ppa.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_types.h"
#include "sdkconfig.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "esp_extractor.h"
#include "esp_video_dec.h"

namespace esp_brookesia::apps
{

    class VideoPlayer : public systems::phone::App
    {
    public:
        static VideoPlayer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~VideoPlayer();

    protected:
        VideoPlayer(bool use_status_bar, bool use_navigation_bar);
        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static VideoPlayer *_instance;

        static constexpr int SWIPE_EXIT_THRESHOLD = 180;

        lv_display_t *display = nullptr;
        esp_lcd_panel_handle_t display_panel = nullptr;
        ppa_client_handle_t ppa_srm_handle = nullptr;
        esp_lcd_touch_handle_t touch_handle = nullptr;
        /* Panel geometry and the pixel formats matching it, loaded by initDisplayBypass(). */
        board_display_info_t display_info = {};
        uint32_t bytes_per_pixel = 2;
        ppa_srm_color_mode_t ppa_color_mode = PPA_SRM_COLOR_MODE_RGB565;
        esp_video_codec_pixel_fmt_t decode_format = ESP_VIDEO_CODEC_PIXEL_FMT_RGB565_LE;
        void *lcd_buffers[BOARD_DISPLAY_MAX_FRAME_BUFFERS] = {};
        int current_buf_idx = 0;
        size_t data_cache_line_size = 64;
        uint16_t last_video_width = 0;
        uint16_t last_video_height = 0;
        std::atomic<bool> loop_playback{false};
        std::atomic<bool> is_paused{false};
        bool sd_mounted = false;
        std::atomic<bool> audio_ready{false};
        bool dummy_enabled = false;
        bool lvgl_paused = false;
        bool touch_active = false;
        uint16_t touch_start_x = 0;
        uint16_t touch_start_y = 0;

        /* Owned by the GUI task. The worker only publishes these atomics. */
        TaskHandle_t play_task_handle = nullptr;
        lv_timer_t *close_request_timer = nullptr;
        std::atomic<bool> playback_exited{true};
        std::atomic<bool> close_requested{false};
        std::atomic<bool> close_notification_posted{false};

        /* esp_video_codec MJPEG decoders: hardware first, software as fallback. Both write
         * into the one output buffer, sized and aligned for the stricter hardware path. */
        esp_video_dec_handle_t video_dec_hw = nullptr;
        esp_video_dec_handle_t video_dec_sw = nullptr;
        uint8_t *decode_buffer = nullptr;
        uint32_t decode_buffer_size = 0;
        uint8_t decode_buffer_align = 0;
        bool hw_fallback_logged = false;

        char **avi_file_list = nullptr;
        int avi_file_count = 0;

        lv_obj_t *status_label = nullptr;

        bool startPlaybackTask();
        bool stopPlaybackTask(TickType_t timeout);
        bool createCloseRequestTimer();
        void destroyCloseRequestTimer();
        void releasePlaybackResources();
        esp_err_t getAviFileList(const char *dir_path);
        esp_err_t initDisplayBypass();
        void deinitDisplayBypass();
        esp_err_t initVideoDecoder();
        void deinitVideoDecoder();
        esp_err_t ensureDecodeBuffer(uint32_t size);
        void releaseDecodeBuffer();
        /* A decoded frame: `buffer` holds pic_w x pic_h pixels (padded to the JPEG MCU), of
         * which the top-left width x height are the visible picture. */
        struct DecodedFrame {
            uint8_t *buffer;
            uint32_t pic_w;
            uint32_t pic_h;
            uint32_t width;
            uint32_t height;
            bool from_hw;
        };
        esp_err_t decodeFrame(bool use_hw, const uint8_t *jpeg, uint32_t jpeg_size, DecodedFrame &frame);
        bool shouldExitBySwipe();

        /* One file being played: demux in playTask, audio and video in their own tasks. */
        struct Playback;
        void playFile(const char *path, bool &request_close);
        bool playbackInterrupted(Playback &pb, bool &request_close);
        void renderJpegFrame(const uint8_t *jpeg, uint32_t jpeg_size);

        static void playTask(void *arg);
        static void audioTask(void *arg);
        static void videoTask(void *arg);
        static void closeRequestTimerCallback(lv_timer_t *timer);
    };

} // namespace esp_brookesia::apps
