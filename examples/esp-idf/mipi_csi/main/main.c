/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_board_manager_includes.h"
#include "esp_capture_service_ops.h"
#include "esp_service.h"
#include "esp_video_capture_service.h"
#include "esp_video_capture_service_setup.h"
#include "esp_video_render.h"
#include "esp_video_render_backend.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_video_ppa.h"
#if CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT
#include "esp_lvgl_port.h"
#include "esp_lcd_mipi_dsi.h"
#endif

static const char *TAG = "app_main";

static void check_render(esp_video_render_err_t ret)
{
    if (ret != ESP_VIDEO_RENDER_ERR_OK) {
        ESP_LOGE(TAG, "Video render failed: %d", ret);
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

typedef struct {
    dev_display_lcd_config_t display_config;
    dev_display_lcd_handles_t *lcd_handles;
    esp_capture_service_t *capture;
    esp_capture_video_info_t format;
    esp_gmf_pool_handle_t pool;
    esp_video_render_handle_t render;
    esp_video_render_stream_handle_t stream;
#if CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT
    lv_display_t *display;
#endif
} preview_context_t;

static void init_board(preview_context_t *ctx)
{
    dev_display_lcd_config_t *lcd_config = NULL;
    ESP_ERROR_CHECK(esp_board_manager_get_device_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&lcd_config));
    ESP_ERROR_CHECK(lcd_config && strcmp(lcd_config->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_DSI) == 0 &&
                    lcd_config->lcd_width && lcd_config->lcd_height &&
                    !lcd_config->swap_xy && !lcd_config->mirror_x && !lcd_config->mirror_y &&
                    lcd_config->frame_format == DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_LE
                    ? ESP_OK : ESP_ERR_NOT_SUPPORTED);

    // Use two LCD buffers for the LCD backend or LVGL DSI direct mode.
    ctx->display_config = *lcd_config;
    ctx->display_config.sub_cfg.dsi.dpi_config.num_fbs = 2;
    ESP_ERROR_CHECK(esp_board_device_override_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, &ctx->display_config, sizeof(ctx->display_config)));
    ESP_ERROR_CHECK(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD));
    ESP_ERROR_CHECK(esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD, (void **)&ctx->lcd_handles));
    ESP_ERROR_CHECK(ctx->lcd_handles && ctx->lcd_handles->panel_handle ? ESP_OK : ESP_ERR_INVALID_STATE);
    ESP_ERROR_CHECK(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS));
    ESP_ERROR_CHECK(esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_CAMERA));
}

static void init_capture(preview_context_t *ctx)
{
    esp_video_capture_service_cfg_t capture_config = ESP_VIDEO_CAPTURE_SERVICE_CFG_DEFAULT();
    capture_config.video_dev_name = ESP_BOARD_DEVICE_NAME_CAMERA;
    ESP_ERROR_CHECK(esp_video_capture_service_create(&capture_config, &ctx->capture));
    ESP_ERROR_CHECK(esp_video_capture_service_get_default_format(ctx->capture, &ctx->format));
    ESP_ERROR_CHECK(ctx->format.width && ctx->format.height ? ESP_OK : ESP_ERR_INVALID_SIZE);
    if (ctx->format.fps == 0) {
        ctx->format.fps = 30;
    }
    esp_video_capture_service_setup_t setup = {
        .stream_num = 1,
        // Leave capture buffers available while the application renders a held frame.
        .fb_num = 3,
        .streams[0] = {
            .enabled = true,
            .video_info = {
                .codec = ESP_CAPTURE_FMT_ID_RGB565,
                .width = ctx->format.width,
                .height = ctx->format.height,
                .fps = ctx->format.fps,
            },
        },
    };
    ESP_ERROR_CHECK(esp_video_capture_service_apply_setup(ctx->capture, &setup));
}

static void init_gmf_pool(preview_context_t *ctx)
{
    // Use the GMF PPA element for hardware scaling, as in the official P4 example.
    ESP_ERROR_CHECK(esp_gmf_pool_init(&ctx->pool) == ESP_GMF_ERR_OK ? ESP_OK : ESP_FAIL);
    esp_gmf_element_handle_t scale = NULL;
    ESP_ERROR_CHECK(esp_gmf_video_ppa_init(NULL, &scale) == ESP_GMF_ERR_OK ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(esp_gmf_pool_register_element(ctx->pool, scale, NULL) == ESP_GMF_ERR_OK ? ESP_OK : ESP_FAIL);
}

static void init_display_backend(preview_context_t *ctx)
{
#if CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT
    lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port_config));
    lvgl_port_display_cfg_t display_port = {
        .io_handle = ctx->lcd_handles->io_handle,
        .panel_handle = ctx->lcd_handles->panel_handle,
        .buffer_size = (uint32_t)ctx->display_config.lcd_width * ctx->display_config.lcd_height,
        .double_buffer = true,
        .hres = ctx->display_config.lcd_width,
        .vres = ctx->display_config.lcd_height,
        .rotation = {
            .swap_xy = ctx->display_config.swap_xy,
            .mirror_x = ctx->display_config.mirror_x,
            .mirror_y = ctx->display_config.mirror_y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .direct_mode = true,
            .buff_spiram = true,
        },
    };
    const lvgl_port_display_dsi_cfg_t dsi_port = {
        .flags.avoid_tearing = true,
    };
    ctx->display = lvgl_port_add_disp_dsi(&display_port, &dsi_port);
    ESP_ERROR_CHECK(ctx->display ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(lvgl_port_lock(0) ? ESP_OK : ESP_ERR_TIMEOUT);
    lv_obj_set_style_bg_color(lv_display_get_screen_active(ctx->display), lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lv_display_get_screen_active(ctx->display), LV_OPA_COVER, 0);
    lvgl_port_unlock();
    esp_video_render_lvgl_cfg_t lvgl_backend = {
        .lv_disp = ctx->display,
        .out_format = ESP_VIDEO_RENDER_FORMAT_RGB565,
        .width = ctx->display_config.lcd_width,
        .height = ctx->display_config.lcd_height,
    };
    esp_video_render_backend_cfg_t backend = {
        .ops = esp_video_render_get_lvgl_backend(),
        .cfg = &lvgl_backend,
        .cfg_size = sizeof(lvgl_backend),
    };
#else
    esp_video_render_lcd_cfg_t lcd_backend = {
        .lcd_type = ESP_VIDEO_RENDER_LCD_TYPE_DPI,
        .fb_num = 2,
        .out_format = ESP_VIDEO_RENDER_FORMAT_RGB565,
        .width = ctx->display_config.lcd_width,
        .height = ctx->display_config.lcd_height,
        .lcd_handle = ctx->lcd_handles->panel_handle,
        .io_handle = ctx->lcd_handles->io_handle,
    };
    esp_video_render_backend_cfg_t backend = {
        .ops = esp_video_render_get_lcd_backend(),
        .cfg = &lcd_backend,
        .cfg_size = sizeof(lcd_backend),
    };
#endif
    check_render(esp_video_render_set_display(ctx->render, &backend));
}

static void init_render_stream(preview_context_t *ctx)
{
    esp_video_render_clr_t black = {0};
    check_render(esp_video_render_set_bg_color(ctx->render, &black));
    esp_video_render_stream_info_t stream_info = {
        .info = {
            .format = ESP_VIDEO_RENDER_FORMAT_RGB565,
            .width = ctx->format.width,
            .height = ctx->format.height,
            .fps = ctx->format.fps,
        },
        // A single stream renders synchronously; avoid an extra full-frame copy.
        .cached = false,
    };
    check_render(esp_video_render_stream_open(ctx->render, &stream_info, &ctx->stream));

    // Fit the whole camera frame on screen, preserving its aspect ratio.
    uint32_t width = ctx->display_config.lcd_width;
    uint32_t height = width * ctx->format.height / ctx->format.width;
    if (height > ctx->display_config.lcd_height) {
        height = ctx->display_config.lcd_height;
        width = height * ctx->format.width / ctx->format.height;
    }
    // Keep the fitted RGB565 dimensions even.
    width &= ~1U;
    height &= ~1U;
    ESP_ERROR_CHECK(width && height ? ESP_OK : ESP_ERR_INVALID_SIZE);
    esp_video_render_rect_t rect = {
        .x = (ctx->display_config.lcd_width - width) / 2,
        .y = (ctx->display_config.lcd_height - height) / 2,
        .width = width,
        .height = height,
    };
    check_render(esp_video_render_stream_set_disp_rect(ctx->stream, &rect));
}

static void init_renderer(preview_context_t *ctx)
{
    init_gmf_pool(ctx);
    esp_video_render_cfg_t render_config = {
        .pool = ctx->pool,
        .fps = ctx->format.fps,
    };
    check_render(esp_video_render_create(&render_config, &ctx->render));
    init_display_backend(ctx);
    init_render_stream(ctx);
}

static void run_preview(preview_context_t *ctx)
{
    ESP_ERROR_CHECK(esp_service_start(ESP_SERVICE_BASE(ctx->capture)));
#if CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT
    ESP_LOGI(TAG, "RGB565 capture service -> video render LVGL backend (DSI direct mode)");
#else
    ESP_LOGI(TAG, "RGB565 capture service -> video render LCD backend");
#endif

    int64_t stats_start = esp_timer_get_time();
    int64_t render_time_us = 0;
    int64_t acquire_time_us = 0;
    unsigned frame_count = 0;
    while (true) {
        esp_media_frame_t frame = {.type = ESP_MEDIA_TRACK_TYPE_VIDEO};
        int64_t acquire_start = esp_timer_get_time();
        esp_err_t ret = esp_capture_service_acquire_frame(ctx->capture, 0, &frame, 1000);
        if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Camera frame timeout");
            continue;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Acquire frame failed: %s", esp_err_to_name(ret));
            break;
        }
        acquire_time_us += esp_timer_get_time() - acquire_start;
        esp_video_render_frame_t render_frame = {
            .format = ESP_VIDEO_RENDER_FORMAT_RGB565,
            .width = ctx->format.width,
            .height = ctx->format.height,
            .data = frame.data,
            .size = frame.size,
            .pts = frame.pts,
        };
        int64_t render_start = esp_timer_get_time();
        esp_video_render_err_t render_ret = ESP_VIDEO_RENDER_ERR_INVALID_ARG;
        if (frame.data && frame.size >= (size_t)ctx->format.width * ctx->format.height * 2) {
            render_ret = esp_video_render_stream_write(ctx->stream, &render_frame);
        }
        render_time_us += esp_timer_get_time() - render_start;
        // With one stream and no async renderer, stream_write consumes the frame before returning.
        ret = esp_capture_service_release_frame(ctx->capture, 0, &frame);
        if (render_ret != ESP_VIDEO_RENDER_ERR_OK || ret != ESP_OK) {
            ESP_LOGE(TAG, "Preview failed: render=%d release=%s", render_ret, esp_err_to_name(ret));
            break;
        }
        ++frame_count;
        int64_t elapsed = esp_timer_get_time() - stats_start;
        if (elapsed >= 2000000) {
            unsigned fps_x10 = (unsigned)((int64_t)frame_count * 10000000 / elapsed);
            ESP_LOGI(TAG, "Preview: %u.%u FPS, average acquire %u ms, render %u ms/frame",
                     fps_x10 / 10, fps_x10 % 10,
                     (unsigned)(acquire_time_us / frame_count / 1000),
                     (unsigned)(render_time_us / frame_count / 1000));
            stats_start = esp_timer_get_time();
            render_time_us = 0;
            acquire_time_us = 0;
            frame_count = 0;
        }
    }
    ESP_ERROR_CHECK(esp_service_stop(ESP_SERVICE_BASE(ctx->capture)));
}

static void deinit_preview(preview_context_t *ctx)
{
    check_render(esp_video_render_stream_close(ctx->stream));
    check_render(esp_video_render_destroy(ctx->render));
#if CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT
    // Wait for any active LVGL refresh before detaching the display callbacks.
    ESP_ERROR_CHECK(lvgl_port_lock(0) ? ESP_OK : ESP_ERR_TIMEOUT);
    ESP_ERROR_CHECK(lvgl_port_stop());
    const esp_lcd_dpi_panel_event_callbacks_t no_callbacks = {0};
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(ctx->lcd_handles->panel_handle, &no_callbacks, NULL));
    ESP_ERROR_CHECK(lvgl_port_remove_disp(ctx->display));
    lvgl_port_unlock();
    ESP_ERROR_CHECK(lvgl_port_deinit());
#endif
    ESP_ERROR_CHECK(esp_gmf_pool_deinit(ctx->pool) == ESP_GMF_ERR_OK ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(esp_capture_service_destroy(ctx->capture));
    ESP_ERROR_CHECK(esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_CAMERA));
    ESP_ERROR_CHECK(esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_LCD_BRIGHTNESS));
    ESP_ERROR_CHECK(esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD));
    ESP_ERROR_CHECK(esp_board_device_restore_config(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD));
}

void app_main(void)
{
    esp_log_level_set("VIDEO_RENDER", ESP_LOG_ERROR);
    preview_context_t ctx = {0};
    init_board(&ctx);
    init_capture(&ctx);
    init_renderer(&ctx);
    run_preview(&ctx);
    deinit_preview(&ctx);
}
