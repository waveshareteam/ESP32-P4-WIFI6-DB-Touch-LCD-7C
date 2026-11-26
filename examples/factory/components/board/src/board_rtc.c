/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "board_rtc.h"

#include "driver/i2c_master.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "pcf85063a.h"

#if __has_include("gen_board_device_custom.h")
#include "gen_board_device_custom.h"
#else
typedef struct {
    const char *name;
    const char *type;
    const char *chip;
    int8_t       i2c_addr;
    int8_t       int_gpio_num;
    uint8_t      peripheral_count;
    const char  *peripheral_names[4];
} dev_custom_rtc_config_t;
#endif

#define BOARD_RTC_DEV_NAME        "rtc"
#define BOARD_RTC_I2C_PERIPH_NAME ESP_BOARD_PERIPH_NAME_I2C_MASTER

static const char *TAG = "board_rtc";

static pcf85063a_dev_t s_rtc;
static bool s_rtc_ready;
static void *s_i2c_master;
static SemaphoreHandle_t s_rtc_mutex;
static portMUX_TYPE s_rtc_init_lock = portMUX_INITIALIZER_UNLOCKED;

static esp_err_t board_rtc_ensure_mutex(void)
{
    if (s_rtc_mutex) {
        return ESP_OK;
    }
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(mutex, ESP_ERR_NO_MEM, TAG, "Create RTC mutex failed");

    portENTER_CRITICAL(&s_rtc_init_lock);
    if (!s_rtc_mutex) {
        s_rtc_mutex = mutex;
        mutex = NULL;
    }
    portEXIT_CRITICAL(&s_rtc_init_lock);

    if (mutex) {
        vSemaphoreDelete(mutex);
    }
    return ESP_OK;
}

static esp_err_t board_rtc_ensure(void)
{
    ESP_RETURN_ON_ERROR(board_rtc_ensure_mutex(), TAG, "RTC mutex unavailable");

    xSemaphoreTake(s_rtc_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (!s_rtc_ready) {
        const char *periph_name = BOARD_RTC_I2C_PERIPH_NAME;
        uint8_t i2c_addr = PCF85063A_ADDRESS;
        if (esp_board_manager_check_name(BOARD_RTC_DEV_NAME)) {
            dev_custom_rtc_config_t *cfg = NULL;
            if (esp_board_manager_get_device_config(BOARD_RTC_DEV_NAME, (void **)&cfg) == ESP_OK && cfg) {
                if (cfg->peripheral_count > 0 && cfg->peripheral_names[0]) {
                    periph_name = cfg->peripheral_names[0];
                }
                if (cfg->i2c_addr > 0) {
                    i2c_addr = (uint8_t)cfg->i2c_addr;
                }
            }
        }
        // Keep the i2c_master reference for good so other devices do not tear the shared bus down.
        if (!s_i2c_master) {
            ret = esp_board_periph_ref_handle(periph_name, &s_i2c_master);
        }
        if (ret == ESP_OK) {
            ret = pcf85063a_init(&s_rtc, (i2c_master_bus_handle_t)s_i2c_master, i2c_addr);
        }
        if (ret == ESP_OK) {
            s_rtc_ready = true;
        }
    }
    xSemaphoreGive(s_rtc_mutex);
    ESP_RETURN_ON_ERROR(ret, TAG, "Initialize PCF85063A failed");
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

esp_err_t board_rtc_init(void)
{
    return board_rtc_ensure();
}

esp_err_t board_rtc_deinit(void)
{
    if (!s_rtc_mutex) {
        return ESP_OK;
    }
    xSemaphoreTake(s_rtc_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (s_rtc_ready && s_rtc.dev_handle) {
        ret = i2c_master_bus_rm_device(s_rtc.dev_handle);
    }
    s_rtc_ready = false;
    memset(&s_rtc, 0, sizeof(s_rtc));
    xSemaphoreGive(s_rtc_mutex);
    return ret;
}

esp_err_t board_rtc_get_int_gpio(gpio_num_t *int_gpio)
{
    ESP_RETURN_ON_FALSE(int_gpio, ESP_ERR_INVALID_ARG, TAG, "Invalid int_gpio pointer");
    dev_custom_rtc_config_t *cfg = NULL;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_config(BOARD_RTC_DEV_NAME, (void **)&cfg),
                        TAG, "Get %s config failed", BOARD_RTC_DEV_NAME);
    ESP_RETURN_ON_FALSE(cfg, ESP_ERR_INVALID_STATE, TAG, "Invalid %s config", BOARD_RTC_DEV_NAME);
    *int_gpio = (gpio_num_t)cfg->int_gpio_num;
    return ESP_OK;
}

esp_err_t board_rtc_sync_system_time(void)
{
    board_rtc_time_t rtc_time = {0};
    ESP_RETURN_ON_ERROR(board_rtc_get_time(&rtc_time), TAG, "Read RTC time failed");

    if (rtc_time.year < 2024 || rtc_time.year > 2099 ||
            rtc_time.month < 1 || rtc_time.month > 12 ||
            rtc_time.day < 1 || rtc_time.day > 31 ||
            rtc_time.hour > 23 || rtc_time.minute > 59 || rtc_time.second > 59) {
        ESP_LOGW(TAG, "RTC time %04u-%02u-%02u is out of valid range, skip system time sync",
                 rtc_time.year, rtc_time.month, rtc_time.day);
        return ESP_ERR_INVALID_RESPONSE;
    }

    struct tm tm_info = {
        .tm_year = (int)rtc_time.year - 1900,
        .tm_mon = (int)rtc_time.month - 1,
        .tm_mday = (int)rtc_time.day,
        .tm_wday = (int)rtc_time.weekday,
        .tm_hour = (int)rtc_time.hour,
        .tm_min = (int)rtc_time.minute,
        .tm_sec = (int)rtc_time.second,
        .tm_isdst = -1,
    };
    time_t epoch = mktime(&tm_info);
    if (epoch < 0) {
        ESP_LOGW(TAG, "Convert RTC time to epoch failed");
        return ESP_ERR_INVALID_RESPONSE;
    }

    struct timeval tv = {
        .tv_sec = epoch,
        .tv_usec = 0,
    };
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGW(TAG, "settimeofday failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "System time synced from RTC: %04u-%02u-%02u %02u:%02u:%02u",
             rtc_time.year, rtc_time.month, rtc_time.day,
             rtc_time.hour, rtc_time.minute, rtc_time.second);
    return ESP_OK;
}
