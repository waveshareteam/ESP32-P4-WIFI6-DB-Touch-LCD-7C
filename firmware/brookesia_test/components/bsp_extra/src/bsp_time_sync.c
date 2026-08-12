#include "bsp_time_sync.h"

#include "bsp/esp-bsp.h"
#include "esp_err.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:TimeSync"
#include "esp_lib_utils.h"

static atomic_bool s_rtc_sync_task_running;

static void rtc_sync_task(void *arg)
{
    (void)arg;
    time_t now = time(NULL);
    struct tm local_time = {0};
    localtime_r(&now, &local_time);

    pcf85063a_datetime_t rtc_time = {
        .sec = (uint8_t)local_time.tm_sec,
        .min = (uint8_t)local_time.tm_min,
        .hour = (uint8_t)local_time.tm_hour,
        .day = (uint8_t)local_time.tm_mday,
        .dotw = (uint8_t)local_time.tm_wday,
        .month = (uint8_t)(local_time.tm_mon + 1),
        .year = (uint16_t)(local_time.tm_year + 1900),
    };
    esp_err_t err = bsp_rtc_set_time(&rtc_time);
    if (err == ESP_OK) {
        ESP_UTILS_LOGI("RTC synchronized from system time");
    } else {
        ESP_UTILS_LOGW("Synchronize RTC failed: %s", esp_err_to_name(err));
    }

    atomic_store(&s_rtc_sync_task_running, false);
    vTaskDelete(NULL);
}

static void time_sync_notification_cb(struct timeval *tv)
{
    (void)tv;
    setenv("TZ", "CST-8", 1);
    tzset();
    ESP_UTILS_LOGI("SNTP time synced, timezone CST-8 applied");

    bool expected = false;
    if (!atomic_compare_exchange_strong(
            &s_rtc_sync_task_running,
            &expected,
            true
        )) {
        return;
    }

    BaseType_t task_ret = xTaskCreate(
        rtc_sync_task, "rtc_sync", 4 * 1024, NULL, 5, NULL
    );
    if (task_ret != pdPASS) {
        atomic_store(&s_rtc_sync_task_running, false);
        ESP_UTILS_LOGW("Create RTC synchronization task failed");
    }
}

void bsp_extra_time_sync_start(void)
{
    if (esp_sntp_enabled()) {
        return;
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    ESP_UTILS_LOGI("SNTP time sync started");
}
