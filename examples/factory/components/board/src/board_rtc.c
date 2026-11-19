/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_rtc.h"

#include "driver/i2c_master.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "pcf85063a.h"

static const char *TAG = "board_rtc";

static pcf85063a_dev_t s_rtc;
static bool s_rtc_ready;
static void *s_i2c_master;
static SemaphoreHandle_t s_rtc_mutex;

static esp_err_t board_rtc_ensure(void)
{
    if (!s_rtc_mutex) {
        s_rtc_mutex = xSemaphoreCreateMutex();
        ESP_RETURN_ON_FALSE(s_rtc_mutex, ESP_ERR_NO_MEM, TAG, "Create RTC mutex failed");
    }
    if (s_rtc_ready) {
        return ESP_OK;
    }
    // Keep the i2c_master reference for good so other devices do not tear the shared bus down.
    if (!s_i2c_master) {
        ESP_RETURN_ON_ERROR(esp_board_periph_ref_handle("i2c_master", &s_i2c_master), TAG, "Initialize i2c_master failed");
    }
    ESP_RETURN_ON_ERROR(pcf85063a_init(&s_rtc, (i2c_master_bus_handle_t)s_i2c_master, PCF85063A_ADDRESS), TAG,
                        "Initialize PCF85063A failed");
    s_rtc_ready = true;
    return ESP_OK;
}

esp_err_t board_rtc_get_time(board_rtc_time_t *time)
{
    ESP_RETURN_ON_FALSE(time, ESP_ERR_INVALID_ARG, TAG, "Time is NULL");
    ESP_RETURN_ON_ERROR(board_rtc_ensure(), TAG, "RTC unavailable");

    xSemaphoreTake(s_rtc_mutex, portMAX_DELAY);
    pcf85063a_datetime_t raw = {0};
    esp_err_t ret = pcf85063a_get_time_date(&s_rtc, &raw);
    xSemaphoreGive(s_rtc_mutex);
    ESP_RETURN_ON_ERROR(ret, TAG, "Read RTC failed");

    *time = (board_rtc_time_t){
        .year = raw.year,
        .month = raw.month,
        .day = raw.day,
        .weekday = raw.dotw,
        .hour = raw.hour,
        .minute = raw.min,
        .second = raw.sec,
    };
    return ESP_OK;
}

esp_err_t board_rtc_set_time(const board_rtc_time_t *time)
{
    ESP_RETURN_ON_FALSE(time, ESP_ERR_INVALID_ARG, TAG, "Time is NULL");
    ESP_RETURN_ON_FALSE(time->year >= 2000 && time->year <= 2099 && time->month >= 1 && time->month <= 12 &&
                        time->day >= 1 && time->day <= 31 && time->weekday <= 6 && time->hour <= 23 &&
                        time->minute <= 59 && time->second <= 59,
                        ESP_ERR_INVALID_ARG, TAG, "Time out of range");
    ESP_RETURN_ON_ERROR(board_rtc_ensure(), TAG, "RTC unavailable");

    const pcf85063a_datetime_t raw = {
        .year = time->year,
        .month = time->month,
        .day = time->day,
        .dotw = time->weekday,
        .hour = time->hour,
        .min = time->minute,
        .sec = time->second,
    };
    xSemaphoreTake(s_rtc_mutex, portMAX_DELAY);
    esp_err_t ret = pcf85063a_set_time_date(&s_rtc, raw);
    xSemaphoreGive(s_rtc_mutex);
    return ret;
}
