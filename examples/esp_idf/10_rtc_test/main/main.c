#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcf85063a.h"

#ifndef RTC_TEST_ENABLE_INTERRUPT
#define RTC_TEST_ENABLE_INTERRUPT 0
#endif

#if RTC_TEST_ENABLE_INTERRUPT
#include "freertos/semphr.h"
#endif

#define RTC_I2C_PORT       I2C_NUM_0
#define RTC_I2C_SDA_GPIO   GPIO_NUM_7
#define RTC_I2C_SCL_GPIO   GPIO_NUM_8
#if RTC_TEST_ENABLE_INTERRUPT
#define RTC_INT_GPIO       GPIO_NUM_20
#endif
#define RTC_TIMER_SECONDS  5
#define RTC_ALARM_WAIT_SEC 8

static const char *TAG = "rtc_test";
#if RTC_TEST_ENABLE_INTERRUPT
static SemaphoreHandle_t s_rtc_int_sem;

static void IRAM_ATTR rtc_int_isr(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)arg, &higher_priority_task_woken);
    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t rtc_int_init(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << RTC_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "configure RTC INT GPIO");
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "install GPIO ISR service");
    return gpio_isr_handler_add(RTC_INT_GPIO, rtc_int_isr, s_rtc_int_sem);
}
#endif

static esp_err_t rtc_i2c_bus_init(i2c_master_bus_handle_t *bus)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = RTC_I2C_PORT,
        .sda_io_num = RTC_I2C_SDA_GPIO,
        .scl_io_num = RTC_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&config, bus);
}

static bool rtc_time_is_valid(const pcf85063a_datetime_t *time)
{
    return time->year >= 2024 && time->year <= 2099 &&
           time->month >= 1 && time->month <= 12 &&
           time->day >= 1 && time->day <= 31 &&
           time->hour <= 23 && time->min <= 59 && time->sec <= 59;
}

static uint8_t build_time_weekday(int year, int month, int day)
{
    static const uint8_t days_before_month[] = {0, 3, 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2};
    int adjusted_year = year - (month < 3);
    return (uint8_t)((adjusted_year + adjusted_year / 4 - adjusted_year / 100 +
                      adjusted_year / 400 + days_before_month[month] + day) % 7);
}

static pcf85063a_datetime_t build_time(void)
{
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month_text[4] = {0};
    pcf85063a_datetime_t time = {0};
    int month = 0;

    sscanf(__DATE__, "%3s %hhu %hu", month_text, &time.day, &time.year);
    sscanf(__TIME__, "%hhu:%hhu:%hhu", &time.hour, &time.min, &time.sec);
    const char *month_pos = strstr(months, month_text);
    if (month_pos != NULL) {
        month = (int)((month_pos - months) / 3) + 1;
    }
    time.month = (uint8_t)month;
    time.dotw = build_time_weekday(time.year, time.month, time.day);
    return time;
}

static void add_seconds(pcf85063a_datetime_t *time, uint8_t seconds)
{
    time->sec += seconds;
    if (time->sec >= 60) {
        time->sec -= 60;
        time->min++;
    }
    if (time->min >= 60) {
        time->min -= 60;
        time->hour++;
    }
    if (time->hour >= 24) {
        time->hour = 0;
        time->day++;
        time->dotw = (uint8_t)((time->dotw + 1) % 7);
    }
}

static void log_rtc_time(const char *label, const pcf85063a_datetime_t *time)
{
    char text[32];
    pcf85063a_datetime_to_str(text, sizeof(text), *time);
    ESP_LOGI(TAG, "%s: %s", label, text);
}

static esp_err_t expect(bool condition, const char *name)
{
    if (!condition) {
        ESP_LOGE(TAG, "FAIL: %s", name);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "PASS: %s", name);
    return ESP_OK;
}

static esp_err_t test_register_roundtrip(pcf85063a_dev_t *rtc)
{
    uint8_t original_offset = 0;
    uint8_t original_ram = 0;
    uint8_t value = 0;
    uint8_t write_data[2];

    ESP_LOGI(TAG, "Testing offset and RAM register read/write");
    ESP_RETURN_ON_ERROR(pcf85063a_read_register(rtc, PCF85063A_RTC_OFFSET_ADDR,
                                                &original_offset, 1), TAG, "read offset");
    ESP_RETURN_ON_ERROR(pcf85063a_read_register(rtc, PCF85063A_RTC_RAM_by_ADDR,
                                                &original_ram, 1), TAG, "read RAM");

    write_data[0] = PCF85063A_RTC_OFFSET_ADDR;
    write_data[1] = original_offset;
    ESP_RETURN_ON_ERROR(pcf85063a_write_register(rtc, write_data, 2), TAG, "restore offset");
    ESP_RETURN_ON_ERROR(pcf85063a_read_register(rtc, PCF85063A_RTC_OFFSET_ADDR, &value, 1), TAG, "verify offset");
    ESP_RETURN_ON_ERROR(expect(value == original_offset, "offset register read/write"), TAG, "offset mismatch");

    write_data[0] = PCF85063A_RTC_RAM_by_ADDR;
    write_data[1] = (uint8_t)(original_ram ^ 0x5A);
    ESP_RETURN_ON_ERROR(pcf85063a_write_register(rtc, write_data, 2), TAG, "write RAM");
    ESP_RETURN_ON_ERROR(pcf85063a_read_register(rtc, PCF85063A_RTC_RAM_by_ADDR, &value, 1), TAG, "read RAM");
    ESP_RETURN_ON_ERROR(expect(value == write_data[1], "RAM register read/write"), TAG, "RAM mismatch");

    write_data[1] = original_ram;
    return pcf85063a_write_register(rtc, write_data, 2);
}

static esp_err_t test_time_access(pcf85063a_dev_t *rtc, pcf85063a_datetime_t *baseline)
{
    pcf85063a_datetime_t test_time = *baseline;
    pcf85063a_datetime_t read_back = {0};

    ESP_LOGI(TAG, "Testing separate time/date and combined time/date access");
    test_time.hour = 12;
    test_time.min = 34;
    test_time.sec = 56;
    ESP_RETURN_ON_ERROR(pcf85063a_set_time(rtc, test_time), TAG, "set time");
    ESP_RETURN_ON_ERROR(pcf85063a_get_time_date(rtc, &read_back), TAG, "read time");
    ESP_RETURN_ON_ERROR(expect(read_back.hour == 12 && read_back.min == 34 && read_back.sec == 56,
                               "set/get time"), TAG, "time mismatch");

    test_time.year = 2030;
    test_time.month = 1;
    test_time.day = 2;
    test_time.dotw = 3;
    ESP_RETURN_ON_ERROR(pcf85063a_set_date(rtc, test_time), TAG, "set date");
    ESP_RETURN_ON_ERROR(pcf85063a_get_time_date(rtc, &read_back), TAG, "read date");
    ESP_RETURN_ON_ERROR(expect(read_back.year == 2030 && read_back.month == 1 &&
                               read_back.day == 2 && read_back.dotw == 3,
                               "set/get date"), TAG, "date mismatch");

    ESP_RETURN_ON_ERROR(pcf85063a_set_time_date(rtc, *baseline), TAG, "restore time/date");
    ESP_RETURN_ON_ERROR(pcf85063a_get_time_date(rtc, &read_back), TAG, "read restored time/date");
    return expect(read_back.year == baseline->year && read_back.month == baseline->month &&
                  read_back.day == baseline->day && read_back.dotw == baseline->dotw,
                  "restore time/date");
}

static esp_err_t test_alarm(pcf85063a_dev_t *rtc, const pcf85063a_datetime_t *now)
{
    pcf85063a_datetime_t alarm = *now;
    pcf85063a_datetime_t alarm_read = {0};
    uint8_t alarm_status = 0;
    uint8_t ctrl2 = 0;
    uint8_t write_data[2];
    bool triggered = false;

    add_seconds(&alarm, 3);
    ESP_LOGI(TAG, "Testing alarm in about 3 seconds");
    ESP_RETURN_ON_ERROR(pcf85063a_set_alarm(rtc, alarm), TAG, "set alarm");
    ESP_RETURN_ON_ERROR(pcf85063a_get_alarm(rtc, &alarm_read), TAG, "get alarm");
    ESP_RETURN_ON_ERROR(expect(alarm_read.hour == alarm.hour && alarm_read.min == alarm.min &&
                               alarm_read.sec == alarm.sec, "set/get alarm"), TAG, "alarm mismatch");
#if RTC_TEST_ENABLE_INTERRUPT
    while (xSemaphoreTake(s_rtc_int_sem, 0) == pdTRUE) {}
    ESP_RETURN_ON_ERROR(pcf85063a_enable_alarm(rtc), TAG, "enable alarm");
#else
    write_data[0] = PCF85063A_RTC_CTRL_2_ADDR;
    write_data[1] = PCF85063A_RTC_CTRL_2_DEFAULT;
    ESP_RETURN_ON_ERROR(pcf85063a_write_register(rtc, write_data, 2), TAG, "prepare alarm");
#endif

    for (int i = 0; i < RTC_ALARM_WAIT_SEC; ++i) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_RETURN_ON_ERROR(pcf85063a_get_alarm_flag(rtc, &alarm_status), TAG, "read alarm flag");
        if (alarm_status & PCF85063A_RTC_CTRL_2_AF) {
            triggered = true;
            break;
        }
    }
    ESP_RETURN_ON_ERROR(expect(triggered, "alarm flag trigger"), TAG, "alarm did not trigger");
#if RTC_TEST_ENABLE_INTERRUPT
    ESP_RETURN_ON_ERROR(expect(xSemaphoreTake(s_rtc_int_sem, 0) == pdTRUE, "alarm GPIO20 interrupt"), TAG, "alarm interrupt not received");
#endif

    ESP_RETURN_ON_ERROR(pcf85063a_read_register(rtc, PCF85063A_RTC_CTRL_2_ADDR, &ctrl2, 1), TAG, "read control 2");
    ctrl2 &= (uint8_t)~(PCF85063A_RTC_CTRL_2_AIE | PCF85063A_RTC_CTRL_2_AF);
    write_data[0] = PCF85063A_RTC_CTRL_2_ADDR;
    write_data[1] = ctrl2;
    return pcf85063a_write_register(rtc, write_data, 2);
}

static esp_err_t test_timer(pcf85063a_dev_t *rtc)
{
    uint8_t before = 0;
    uint8_t after = 0;
    uint8_t flag = 0;
    uint8_t timer_mode = PCF85063A_RTC_TIMER_TCF_1HZ;
    bool triggered = false;

#if RTC_TEST_ENABLE_INTERRUPT
    timer_mode |= PCF85063A_RTC_TIMER_MODE_TIE | PCF85063A_RTC_TIMER_MODE_TI_TP;
#endif

    ESP_LOGI(TAG, "Testing timer value, mode, countdown and flag");
    ESP_RETURN_ON_ERROR(pcf85063a_set_timer_value(rtc, RTC_TIMER_SECONDS), TAG, "set timer value");
    ESP_RETURN_ON_ERROR(pcf85063a_set_timer_mode(rtc, timer_mode), TAG, "set timer mode");
    ESP_RETURN_ON_ERROR(pcf85063a_set_timer_safe(rtc, RTC_TIMER_SECONDS, timer_mode), TAG, "enable timer");
#if RTC_TEST_ENABLE_INTERRUPT
    while (xSemaphoreTake(s_rtc_int_sem, 0) == pdTRUE) {}
#endif
    ESP_RETURN_ON_ERROR(pcf85063a_get_timer_value(rtc, &before), TAG, "read timer value");
    vTaskDelay(pdMS_TO_TICKS(1100));
    ESP_RETURN_ON_ERROR(pcf85063a_get_timer_value(rtc, &after), TAG, "read timer countdown");
    ESP_RETURN_ON_ERROR(expect(after < before, "timer countdown"), TAG, "timer did not decrement");

    for (int i = 0; i < RTC_TIMER_SECONDS + 2; ++i) {
        ESP_RETURN_ON_ERROR(pcf85063a_get_timer_flag(rtc, &flag), TAG, "read timer flag");
        if (flag & PCF85063A_RTC_TIMER_FLAG) {
            triggered = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_RETURN_ON_ERROR(expect(triggered, "timer flag trigger"), TAG, "timer did not trigger");
#if RTC_TEST_ENABLE_INTERRUPT
    ESP_RETURN_ON_ERROR(expect(xSemaphoreTake(s_rtc_int_sem, 0) == pdTRUE, "timer GPIO20 interrupt"), TAG, "timer interrupt not received");
#endif
    ESP_RETURN_ON_ERROR(pcf85063a_clear_timer_flag(rtc), TAG, "clear timer flag");
    ESP_RETURN_ON_ERROR(pcf85063a_get_timer_flag(rtc, &flag), TAG, "verify timer flag");
    ESP_RETURN_ON_ERROR(expect((flag & PCF85063A_RTC_TIMER_FLAG) == 0, "timer flag clear"), TAG, "timer flag remains set");
    return pcf85063a_disable_timer(rtc);
}

static esp_err_t test_reset(pcf85063a_dev_t *rtc)
{
    pcf85063a_datetime_t after_reset = {0};
    ESP_LOGI(TAG, "Testing software reset and post-reset RTC access");
    ESP_RETURN_ON_ERROR(pcf85063a_reset(rtc), TAG, "software reset");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_RETURN_ON_ERROR(pcf85063a_get_time_date(rtc, &after_reset), TAG, "read after reset");
    return expect(rtc_time_is_valid(&after_reset), "RTC readable after reset");
}

static void rtc_test_task(void *arg)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)arg;
    pcf85063a_dev_t rtc = {0};
    pcf85063a_datetime_t now = {0};
    esp_err_t ret = pcf85063a_init(&rtc, bus, PCF85063A_ADDRESS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCF85063A init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ret = pcf85063a_get_time_date(&rtc, &now);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initial RTC read failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    log_rtc_time("Initial time", &now);
    if (!rtc_time_is_valid(&now)) {
        now = build_time();
        ESP_LOGW(TAG, "RTC time is invalid; setting build time once");
        ESP_ERROR_CHECK(pcf85063a_set_time_date(&rtc, now));
        ESP_ERROR_CHECK(pcf85063a_get_time_date(&rtc, &now));
        log_rtc_time("After initialization", &now);
    } else {
        ESP_LOGI(TAG, "RTC time is valid; preserving it");
    }

    ESP_ERROR_CHECK(test_register_roundtrip(&rtc));
    ESP_ERROR_CHECK(test_time_access(&rtc, &now));
    ESP_ERROR_CHECK(pcf85063a_get_time_date(&rtc, &now));
    ESP_ERROR_CHECK(test_alarm(&rtc, &now));
    ESP_ERROR_CHECK(test_timer(&rtc));
    ESP_ERROR_CHECK(test_reset(&rtc));

    ESP_LOGI(TAG, "All exposed PCF85063A function tests finished");
    ESP_LOGI(TAG, "Starting 1-second RTC time output");
    while (true) {
        if (pcf85063a_get_time_date(&rtc, &now) == ESP_OK) {
            log_rtc_time("RTC time", &now);
        } else {
            ESP_LOGE(TAG, "Failed to read RTC time");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
#if RTC_TEST_ENABLE_INTERRUPT
    s_rtc_int_sem = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(s_rtc_int_sem != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(rtc_int_init());
#endif

    i2c_master_bus_handle_t bus = NULL;
    ESP_ERROR_CHECK(rtc_i2c_bus_init(&bus));
    xTaskCreate(rtc_test_task, "rtc_test", 6144, bus, 5, NULL);
}
