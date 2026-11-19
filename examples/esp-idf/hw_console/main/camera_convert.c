/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_log.h"
#include "linux/videodev2.h"
#include "camera_convert.h"

static const char *TAG = "TEST_CAMERA_CONVERT";

const char *camera_convert_fourcc_to_str(uint32_t fourcc, char text[5])
{
    text[0] = (char)(fourcc & 0xff);
    text[1] = (char)((fourcc >> 8) & 0xff);
    text[2] = (char)((fourcc >> 16) & 0xff);
    text[3] = (char)((fourcc >> 24) & 0xff);
    text[4] = '\0';
    return text;
}

bool camera_convert_v4l2_to_imgfx_fmt(uint32_t v4l2_fmt, esp_imgfx_pixel_fmt_t *out_fmt)
{
    if (out_fmt == NULL) {
        return false;
    }

    switch (v4l2_fmt) {
        case V4L2_PIX_FMT_RGB565:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
            return true;
        case V4L2_PIX_FMT_RGB565X:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_BE;
            return true;
        case V4L2_PIX_FMT_YUYV:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_YUYV;
            return true;
        case V4L2_PIX_FMT_UYVY:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_UYVY;
            return true;
        case V4L2_PIX_FMT_GREY:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_Y;
            return true;
        case V4L2_PIX_FMT_RGB24:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB888;
            return true;
        case V4L2_PIX_FMT_BGR24:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_BGR888;
            return true;
        case V4L2_PIX_FMT_YUV420:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_I420;
            return true;
        default:
            return false;
    }
}

bool camera_convert_get_tight_stride(uint32_t v4l2_fmt, uint32_t width, uint32_t *stride)
{
    uint32_t bytes_per_pixel = 0;

    if (stride == NULL) {
        return false;
    }

    switch (v4l2_fmt) {
        case V4L2_PIX_FMT_GREY:
            bytes_per_pixel = 1;
            break;
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_RGB565X:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            bytes_per_pixel = 2;
            break;
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_BGR24:
            bytes_per_pixel = 3;
            break;
        default:
            return false;
    }

    *stride = width * bytes_per_pixel;
    return true;
}

esp_err_t camera_convert_pack_rows(uint8_t *dst, const uint8_t *src, size_t src_len,
                                   uint32_t src_stride, uint32_t tight_stride, uint32_t height)
{
    size_t needed_size = (size_t)src_stride * height;

    if (dst == NULL || src == NULL || src_stride < tight_stride || tight_stride == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (src_len < needed_size) {
        ESP_LOGE(TAG, "Camera frame too small for padded rows: len=%u, need=%u",
                 (unsigned)src_len, (unsigned)needed_size);
        return ESP_ERR_INVALID_SIZE;
    }

    for (uint32_t y = 0; y < height; y++) {
        memcpy(dst + y * tight_stride, src + y * src_stride, tight_stride);
    }
    return ESP_OK;
}
