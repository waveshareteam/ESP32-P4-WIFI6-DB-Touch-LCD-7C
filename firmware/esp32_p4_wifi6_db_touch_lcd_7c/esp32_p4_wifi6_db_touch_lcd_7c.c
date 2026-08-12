/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdatomic.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "usb/usb_host.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "esp_codec_dev_defaults.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#include "bsp/esp32_p4_wifi6_db_touch_lcd_7c.h"
#include "bsp_err_check.h"
#include <assert.h>
#include "driver/ledc.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ek79007.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "bsp_touch_firmware_update.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LCD_LEDC_MODE                     (LEDC_LOW_SPEED_MODE)
#define LCD_LEDC_TIMER                    (LEDC_TIMER_1)
#define LCD_LEDC_CHANNEL                  (LEDC_CHANNEL_1)
#define LCD_LEDC_RESOLUTION               (LEDC_TIMER_10_BIT)
#define LCD_LEDC_FREQUENCY                (5000)

static const char *TAG = "ESP32-P4-WIFI6-DB";

static esp_err_t bsp_sdcard_shared_host_init(void)
{
    sdmmc_host_state_t state = {0};
    ESP_RETURN_ON_ERROR(sdmmc_host_get_state(&state), TAG,
                        "Get SDMMC host state failed");
    return state.host_initialized ? ESP_OK : sdmmc_host_init();
}

/**
 * @brief I2C handle for BSP usage
 *
 * In IDF v5.4 you can call i2c_master_get_bus_handle(BSP_I2C_NUM, i2c_master_bus_handle_t *ret_handle)
 * from #include "esp_private/i2c_platform.h" to get this handle
 *
 * For IDF 5.2 and 5.3 you must call bsp_i2c_get_handle()
 */
static i2c_master_bus_handle_t i2c_handle = NULL;
static bool i2c_initialized = false;
static pcf85063a_dev_t rtc_dev = {0};
static bool rtc_device_initialized = false;
static bool backlight_initialized = false;
static int lcd_brightness_percent = 0;
static sdmmc_card_t *bsp_sdcard = NULL;
#if SOC_SDMMC_IO_POWER_EXTERNAL
static sd_pwr_ctrl_handle_t s_sdcard_pwr_ctrl = NULL;
#endif
static i2s_chan_handle_t i2s_tx_chan = NULL;
static i2s_chan_handle_t i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL;
static TaskHandle_t usb_host_task;
static SemaphoreHandle_t usb_host_done_sem;
static atomic_bool usb_host_stop_requested;
static bool user_leds_initialized = false;

static const gpio_num_t header_gpios[] = {
    GPIO_NUM_32,
    GPIO_NUM_33,
    GPIO_NUM_49,
    GPIO_NUM_50,
    GPIO_NUM_52,
    GPIO_NUM_51,
    GPIO_NUM_48,
    GPIO_NUM_47,
    GPIO_NUM_46,
    GPIO_NUM_36,
    GPIO_NUM_25,
    GPIO_NUM_24,
    GPIO_NUM_5,
    GPIO_NUM_4,
};

static const gpio_num_t user_led_gpios[BSP_LED_COUNT] = {
    [BSP_LED_USER1] = BSP_USER1_LED,
    [BSP_LED_USER2] = BSP_USER2_LED,
};

esp_err_t bsp_led_init(void)
{
    if (user_leds_initialized) {
        return ESP_OK;
    }

    const gpio_config_t led_config = {
        .pin_bit_mask = BIT64(BSP_USER1_LED) | BIT64(BSP_USER2_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&led_config), TAG, "Configure user LED GPIOs failed");

    for (bsp_led_t led = BSP_LED_USER1; led < BSP_LED_COUNT; ++led) {
        ESP_RETURN_ON_ERROR(gpio_set_level(user_led_gpios[led], BSP_USER_LED_INACTIVE_LEVEL), TAG,
                            "Turn off user LED failed");
    }
    user_leds_initialized = true;
    return ESP_OK;
}

esp_err_t bsp_led_set(bsp_led_t led, bool enabled)
{
    ESP_RETURN_ON_FALSE((unsigned)led < BSP_LED_COUNT, ESP_ERR_INVALID_ARG, TAG,
                        "Invalid user LED");
    ESP_RETURN_ON_ERROR(bsp_led_init(), TAG, "Initialize user LEDs failed");
    const int level = enabled ? BSP_USER_LED_ACTIVE_LEVEL : BSP_USER_LED_INACTIVE_LEVEL;
    return gpio_set_level(user_led_gpios[led], level);
}

esp_err_t bsp_led_deinit(void)
{
    if (!user_leds_initialized) {
        return ESP_OK;
    }

    for (bsp_led_t led = BSP_LED_USER1; led < BSP_LED_COUNT; ++led) {
        ESP_RETURN_ON_ERROR(gpio_set_level(user_led_gpios[led], BSP_USER_LED_INACTIVE_LEVEL), TAG,
                            "Turn off user LED failed");
    }
    for (bsp_led_t led = BSP_LED_USER1; led < BSP_LED_COUNT; ++led) {
        ESP_RETURN_ON_ERROR(gpio_reset_pin(user_led_gpios[led]), TAG,
                            "Reset user LED GPIO failed");
    }
    user_leds_initialized = false;
    return ESP_OK;
}

const gpio_num_t *bsp_get_header_gpios(size_t *count)
{
    if (count) {
        *count = sizeof(header_gpios) / sizeof(header_gpios[0]);
    }
    return header_gpios;
}

#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_SCLK,  \
        .ws = BSP_I2S_LRCK,    \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DIN,    \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

#define BSP_I2S_DUPLEX_STEREO_CFG(_sample_rate)                                                         \
    {                                                                                                   \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                            \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                   \
    }

#define BSP_ES8389_PGA_LEFT_REG                       (0x72)
#define BSP_ES8389_PGA_RIGHT_REG                      (0x73)
#define BSP_ES8389_PGA_GAIN_MASK                      (0x0F)
#define BSP_ES8389_LEFT_DIFFERENTIAL_INPUT_SEL        (1)
#define BSP_ES8389_RIGHT_DIFFERENTIAL_INPUT_SEL       (1)

esp_err_t bsp_i2c_init(void)
{
    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_config, &i2c_handle));

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    if (!i2c_initialized) {
        return ESP_OK;
    }
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_handle = NULL;
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
    return i2c_handle;
}

static bool rtc_year_is_leap(uint16_t year)
{
    return (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
}

static uint8_t rtc_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && rtc_year_is_leap(year)) {
        return 29;
    }
    return days[month - 1];
}

static bool rtc_datetime_fields_are_valid(const pcf85063a_datetime_t *time)
{
    if (time == NULL || time->year < YEAR_OFFSET || time->year > YEAR_OFFSET + 99 ||
        time->month < 1 || time->month > 12 || time->dotw > 6 ||
        time->hour > 23 || time->min > 59 || time->sec > 59) {
        return false;
    }
    return time->day >= 1 && time->day <= rtc_days_in_month(time->year, time->month);
}

static bool rtc_datetime_is_initialized(const pcf85063a_datetime_t *time)
{
    return rtc_datetime_fields_are_valid(time) && time->year >= 2000;
}

static uint8_t rtc_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t month_offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t adjusted_year = year;

    if (month < 3) {
        adjusted_year--;
    }
    return (uint8_t)((adjusted_year + adjusted_year / 4U - adjusted_year / 100U +
                      adjusted_year / 400U + month_offsets[month - 1] + day) % 7U);
}

static esp_err_t rtc_get_build_time(pcf85063a_datetime_t *time)
{
    static const char month_names[][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    char month_name[4] = {0};
    int year;
    int month = 0;
    int day;
    int hour;
    int minute;
    int second;

    ESP_RETURN_ON_FALSE(time != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid RTC time pointer");
    ESP_RETURN_ON_FALSE(sscanf(__DATE__, "%3s %d %d", month_name, &day, &year) == 3,
                        ESP_ERR_INVALID_STATE, TAG, "Parse build date failed");
    ESP_RETURN_ON_FALSE(sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second) == 3,
                        ESP_ERR_INVALID_STATE, TAG, "Parse build time failed");

    for (size_t i = 0; i < sizeof(month_names) / sizeof(month_names[0]); ++i) {
        if (strcmp(month_name, month_names[i]) == 0) {
            month = (int)i + 1;
            break;
        }
    }
    ESP_RETURN_ON_FALSE(month != 0, ESP_ERR_INVALID_STATE, TAG, "Unknown build month");

    *time = (pcf85063a_datetime_t) {
        .year = (uint16_t)year,
        .month = (uint8_t)month,
        .day = (uint8_t)day,
        .dotw = rtc_day_of_week((uint16_t)year, (uint8_t)month, (uint8_t)day),
        .hour = (uint8_t)hour,
        .min = (uint8_t)minute,
        .sec = (uint8_t)second,
    };
    ESP_RETURN_ON_FALSE(rtc_datetime_fields_are_valid(time), ESP_ERR_INVALID_STATE,
                        TAG, "Build time is outside the PCF85063A range");
    return ESP_OK;
}

static esp_err_t bsp_rtc_device_init(void)
{
    if (rtc_device_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "Initialize RTC I2C bus failed");
    esp_err_t ret = pcf85063a_init(&rtc_dev, i2c_handle, BSP_RTC_I2C_ADDRESS);
    if (ret != ESP_OK) {
        if (rtc_dev.dev_handle != NULL) {
            i2c_master_bus_rm_device(rtc_dev.dev_handle);
        }
        memset(&rtc_dev, 0, sizeof(rtc_dev));
        return ret;
    }

    rtc_device_initialized = true;
    return ESP_OK;
}

esp_err_t bsp_rtc_init(void)
{
    pcf85063a_datetime_t time = {0};

    ESP_RETURN_ON_ERROR(bsp_rtc_device_init(), TAG, "Initialize PCF85063A failed");
    ESP_RETURN_ON_ERROR(pcf85063a_get_time_date(&rtc_dev, &time), TAG, "Read RTC time failed");
    if (rtc_datetime_is_initialized(&time)) {
        ESP_LOGI(TAG, "RTC time is valid; preserving retained time");
        return ESP_OK;
    }

    ESP_LOGW(TAG, "RTC time is invalid; setting firmware build time");
    return bsp_rtc_set_build_time();
}

esp_err_t bsp_rtc_deinit(void)
{
    if (rtc_dev.dev_handle == NULL) {
        rtc_device_initialized = false;
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2c_master_bus_rm_device(rtc_dev.dev_handle), TAG, "Remove PCF85063A from I2C bus failed");
    memset(&rtc_dev, 0, sizeof(rtc_dev));
    rtc_device_initialized = false;
    return ESP_OK;
}

esp_err_t bsp_rtc_set_time(const pcf85063a_datetime_t *time)
{
    ESP_RETURN_ON_FALSE(rtc_datetime_fields_are_valid(time), ESP_ERR_INVALID_ARG,
                        TAG, "Invalid RTC date or time");
    ESP_RETURN_ON_ERROR(bsp_rtc_device_init(), TAG, "Initialize PCF85063A failed");
    return pcf85063a_set_time_date(&rtc_dev, *time);
}

esp_err_t bsp_rtc_get_time(pcf85063a_datetime_t *time)
{
    ESP_RETURN_ON_FALSE(time != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid RTC time pointer");
    ESP_RETURN_ON_ERROR(bsp_rtc_device_init(), TAG, "Initialize PCF85063A failed");
    return pcf85063a_get_time_date(&rtc_dev, time);
}

esp_err_t bsp_rtc_set_build_time(void)
{
    pcf85063a_datetime_t time = {0};

    ESP_RETURN_ON_ERROR(rtc_get_build_time(&time), TAG, "Get firmware build time failed");
    ESP_RETURN_ON_ERROR(bsp_rtc_device_init(), TAG, "Initialize PCF85063A failed");
    ESP_LOGI(TAG, "Set RTC build time: %04u-%02u-%02u %02u:%02u:%02u", (unsigned)time.year, (unsigned)time.month, (unsigned)time.day,
             (unsigned)time.hour, (unsigned)time.min, (unsigned)time.sec);
    return pcf85063a_set_time_date(&rtc_dev, time);
}

pcf85063a_dev_t *bsp_rtc_get_handle(void)
{
    BSP_ERROR_CHECK_RETURN_NULL(bsp_rtc_init());
    return &rtc_dev;
}


esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
        esp_err_t unregister_ret = esp_vfs_spiffs_unregister(conf.partition_label);
        if (unregister_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to roll back SPIFFS mount (%s)", esp_err_to_name(unregister_ret));
        }
    } else {
        ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void)
{
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

sdmmc_card_t *bsp_sdcard_get_handle(void)
{
    return bsp_sdcard;
}

void bsp_sdcard_get_sdmmc_host(const int slot, sdmmc_host_t *config)
{
    assert(config);
    sdmmc_host_t host_config = SDMMC_HOST_DEFAULT();
    host_config.slot = slot;
    host_config.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#if SOC_SDMMC_IO_POWER_EXTERNAL
    host_config.pwr_ctrl_handle = s_sdcard_pwr_ctrl;
#endif
    host_config.init = bsp_sdcard_shared_host_init;
    memcpy(config, &host_config, sizeof(sdmmc_host_t));
}

void bsp_sdcard_sdmmc_get_slot(const int slot, sdmmc_slot_config_t *config)
{
    (void)slot;
    assert(config);
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = BSP_SD_CLK;
    slot_config.cmd = BSP_SD_CMD;
    slot_config.d0 = BSP_SD_D0;
    slot_config.d1 = BSP_SD_D1;
    slot_config.d2 = BSP_SD_D2;
    slot_config.d3 = BSP_SD_D3;
    slot_config.d4 = GPIO_NUM_NC;
    slot_config.d5 = GPIO_NUM_NC;
    slot_config.d6 = GPIO_NUM_NC;
    slot_config.d7 = GPIO_NUM_NC;
    slot_config.cd = SDMMC_SLOT_NO_CD;
    slot_config.wp = SDMMC_SLOT_NO_WP;
    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    memcpy(config, &slot_config, sizeof(sdmmc_slot_config_t));
}

static esp_err_t bsp_sdcard_power_ctrl_init(void)
{
#if SOC_SDMMC_IO_POWER_EXTERNAL
    if (s_sdcard_pwr_ctrl != NULL) {
        return ESP_OK;
    }

    const sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = BSP_SD_PWR_CTRL_LDO_CHAN,
    };
    return sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &s_sdcard_pwr_ctrl);
#else
    return ESP_OK;
#endif
}

static esp_err_t bsp_sdcard_power_ctrl_deinit(void)
{
#if SOC_SDMMC_IO_POWER_EXTERNAL
    if (s_sdcard_pwr_ctrl == NULL) {
        return ESP_OK;
    }

    esp_err_t ret = sd_pwr_ctrl_del_on_chip_ldo(s_sdcard_pwr_ctrl);
    if (ret == ESP_OK) {
        s_sdcard_pwr_ctrl = NULL;
    }
    return ret;
#else
    return ESP_OK;
#endif
}

static esp_err_t bsp_sdcard_power_on(void)
{
    const gpio_config_t power_config = {
        .pin_bit_mask = BIT64(BSP_SD_VDD_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&power_config), TAG, "Configure SD card VDD_EN failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_SD_VDD_EN, BSP_SD_VDD_EN_ACTIVE_LEVEL), TAG, "Enable SD card power failed");
    return ESP_OK;
}

static esp_err_t bsp_sdcard_power_off(void)
{
    return gpio_set_level(BSP_SD_VDD_EN, !BSP_SD_VDD_EN_ACTIVE_LEVEL);
}

esp_err_t bsp_sdcard_sdmmc_mount(bsp_sdcard_cfg_t *cfg)
{
    assert(cfg);
    esp_err_t ret = bsp_sdcard_power_ctrl_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize SD card IO power failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = bsp_sdcard_power_on();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Enable SD card power failed: %s", esp_err_to_name(ret));
        esp_err_t ldo_ret = bsp_sdcard_power_ctrl_deinit();
        if (ldo_ret != ESP_OK) {
            ESP_LOGE(TAG, "Release SD card IO power after power-on failure failed: %s",
                     esp_err_to_name(ldo_ret));
        }
        return ret;
    }
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 64 * 1024,
    };
    sdmmc_host_t host;
    sdmmc_slot_config_t slot_config;
    const esp_vfs_fat_sdmmc_mount_config_t *effective_mount = cfg->mount;
    const sdmmc_host_t *effective_host = cfg->host;
    const sdmmc_slot_config_t *effective_slot = cfg->slot;
    if (effective_mount == NULL) {
        effective_mount = &mount_config;
    }
    if (effective_host == NULL) {
        bsp_sdcard_get_sdmmc_host(SDMMC_HOST_SLOT_0, &host);
        effective_host = &host;
#if SOC_SDMMC_IO_POWER_EXTERNAL
    } else if (s_sdcard_pwr_ctrl != NULL && effective_host->pwr_ctrl_handle == NULL) {
        host = *effective_host;
        host.pwr_ctrl_handle = s_sdcard_pwr_ctrl;
        effective_host = &host;
#endif
    }
    if (effective_slot == NULL) {
        bsp_sdcard_sdmmc_get_slot(SDMMC_HOST_SLOT_0, &slot_config);
        effective_slot = &slot_config;
    }

    ret = esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, effective_host, effective_slot, effective_mount, &bsp_sdcard);
    if (ret != ESP_OK) {
        bsp_sdcard = NULL;
        esp_err_t power_ret = bsp_sdcard_power_off();
        if (power_ret != ESP_OK) {
            ESP_LOGE(TAG, "Disable SD card power after mount failure failed: %s", esp_err_to_name(power_ret));
        }
        esp_err_t ldo_ret = bsp_sdcard_power_ctrl_deinit();
        if (ldo_ret != ESP_OK) {
            ESP_LOGE(TAG, "Release SD card IO power after mount failure failed: %s", esp_err_to_name(ldo_ret));
        }
    }
    return ret;
}

esp_err_t bsp_sdcard_mount(void)
{
    bsp_sdcard_cfg_t cfg = {0};
    return bsp_sdcard_sdmmc_mount(&cfg);
}

esp_err_t bsp_sdcard_unmount(void)
{
    ESP_RETURN_ON_FALSE(bsp_sdcard != NULL, ESP_ERR_INVALID_STATE, TAG, "SD card is not mounted");
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, bsp_sdcard);
    if (ret != ESP_OK) {
        return ret;
    }
    bsp_sdcard = NULL;
    esp_err_t power_ret = bsp_sdcard_power_off();
    esp_err_t ldo_ret = bsp_sdcard_power_ctrl_deinit();
    if (power_ret != ESP_OK) {
        return power_ret;
    }
    return ldo_ret;
}

static void bsp_audio_reset(void)
{
    if (i2s_data_if != NULL) {
        audio_codec_delete_data_if(i2s_data_if);
        i2s_data_if = NULL;
    }
    if (i2s_tx_chan != NULL) {
        (void)i2s_channel_disable(i2s_tx_chan);
        (void)i2s_del_channel(i2s_tx_chan);
        i2s_tx_chan = NULL;
    }
    if (i2s_rx_chan != NULL) {
        (void)i2s_channel_disable(i2s_rx_chan);
        (void)i2s_del_channel(i2s_rx_chan);
        i2s_rx_chan = NULL;
    }
}

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    if (i2s_tx_chan && i2s_rx_chan && i2s_data_if) {
        return ESP_OK;
    }
    bsp_audio_reset();

    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_config.auto_clear = true;
    esp_err_t ret = i2s_new_channel(&chan_config, &i2s_tx_chan, &i2s_rx_chan);
    ESP_GOTO_ON_ERROR(ret, fail, TAG, "Create I2S channels failed");

    const i2s_std_config_t default_config = BSP_I2S_DUPLEX_STEREO_CFG(22050);
    const i2s_std_config_t *config = i2s_config ? i2s_config : &default_config;

    ret = i2s_channel_init_std_mode(i2s_tx_chan, config);
    ESP_GOTO_ON_ERROR(ret, fail, TAG, "Initialize I2S TX failed");
    ret = i2s_channel_init_std_mode(i2s_rx_chan, config);
    ESP_GOTO_ON_ERROR(ret, fail, TAG, "Initialize I2S RX failed");
    ret = i2s_channel_enable(i2s_tx_chan);
    ESP_GOTO_ON_ERROR(ret, fail, TAG, "Enable I2S TX failed");
    ret = i2s_channel_enable(i2s_rx_chan);
    ESP_GOTO_ON_ERROR(ret, fail, TAG, "Enable I2S RX failed");

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = CONFIG_BSP_I2S_NUM,
        .tx_handle = i2s_tx_chan,
        .rx_handle = i2s_rx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (i2s_data_if == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }
    return ESP_OK;

fail:
    bsp_audio_reset();
    return ret;
}

static int bsp_es8389_configure_differential_inputs(esp_codec_dev_handle_t microphone)
{
    int left_gain_reg = 0;
    int right_gain_reg = 0;
    int ret = esp_codec_dev_read_reg(microphone, BSP_ES8389_PGA_LEFT_REG, &left_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }
    ret = esp_codec_dev_read_reg(microphone, BSP_ES8389_PGA_RIGHT_REG, &right_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }

    left_gain_reg = (left_gain_reg & BSP_ES8389_PGA_GAIN_MASK) |
                    (BSP_ES8389_LEFT_DIFFERENTIAL_INPUT_SEL << 4);
    right_gain_reg = (right_gain_reg & BSP_ES8389_PGA_GAIN_MASK) |
                     (BSP_ES8389_RIGHT_DIFFERENTIAL_INPUT_SEL << 4);

    ret = esp_codec_dev_write_reg(microphone, BSP_ES8389_PGA_LEFT_REG, left_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }

    ret = esp_codec_dev_write_reg(microphone, BSP_ES8389_PGA_RIGHT_REG, right_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }

    ret = esp_codec_dev_read_reg(microphone, BSP_ES8389_PGA_LEFT_REG, &left_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }
    ret = esp_codec_dev_read_reg(microphone, BSP_ES8389_PGA_RIGHT_REG, &right_gain_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "ES8389 microphone routes: 0x72=0x%02X (differential), 0x73=0x%02X (differential)",
             left_gain_reg, right_gain_reg);
    return ESP_CODEC_DEV_OK;
}

esp_err_t bsp_audio_codec_configure_microphone(esp_codec_dev_handle_t microphone, float gain_db)
{
    if (microphone == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret = esp_codec_dev_set_in_gain(microphone, gain_db);
    if (ret != ESP_CODEC_DEV_OK) {
        return (esp_err_t)ret;
    }
    ret = bsp_es8389_configure_differential_inputs(microphone);
    if (ret != ESP_CODEC_DEV_OK) {
        return (esp_err_t)ret;
    }

    ESP_LOGI(TAG, "ES8389 configured for two differential microphones");
    return ESP_OK;
}

static esp_codec_dev_handle_t bsp_audio_codec_init(esp_codec_dev_type_t device_type)
{
    if (!i2s_data_if) {
        if (bsp_i2c_init() != ESP_OK || bsp_audio_init(NULL) != ESP_OK) {
            return NULL;
        }
    }

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    if (gpio_if == NULL) {
        return NULL;
    }
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8389_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (ctrl_if == NULL) {
        audio_codec_delete_gpio_if(gpio_if);
        return NULL;
    }
    const esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };
    es8389_codec_cfg_t codec_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = false,
        .digital_mic = false,
        .no_dac_ref = true,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *codec_if = es8389_codec_new(&codec_cfg);
    if (!codec_if) {
        audio_codec_delete_ctrl_if(ctrl_if);
        audio_codec_delete_gpio_if(gpio_if);
        return NULL;
    }
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = device_type,
        .codec_if = codec_if,
        .data_if = i2s_data_if,
    };
    esp_codec_dev_handle_t device = esp_codec_dev_new(&dev_cfg);
    if (device == NULL) {
        audio_codec_delete_codec_if(codec_if);
        audio_codec_delete_ctrl_if(ctrl_if);
        audio_codec_delete_gpio_if(gpio_if);
    }
    return device;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    return bsp_audio_codec_init(ESP_CODEC_DEV_TYPE_OUT);
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    return bsp_audio_codec_init(ESP_CODEC_DEV_TYPE_IN);
}

static const char *LCD_TAG = "BSP-DISPLAY";
static esp_ldo_channel_handle_t s_mipi_phy_ldo;
static esp_lcd_dsi_bus_handle_t s_mipi_dsi_bus;
#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;
#endif
static esp_lcd_touch_handle_t s_touch;
static esp_lcd_panel_io_handle_t s_touch_io;
static bsp_lcd_handles_t s_display_handles;

#define BSP_TOUCH_GT911_PRODUCT_ID_REG          (0x8140)

esp_err_t bsp_display_brightness_init(void)
{
    if (backlight_initialized) {
        return ESP_OK;
    }
    ESP_RETURN_ON_FALSE(BSP_LCD_BACKLIGHT != GPIO_NUM_NC, ESP_ERR_NOT_SUPPORTED, LCD_TAG,
                        "LCD backlight GPIO is not configured");

    const ledc_timer_config_t timer_config = {
        .speed_mode = LCD_LEDC_MODE,
        .duty_resolution = LCD_LEDC_RESOLUTION,
        .timer_num = LCD_LEDC_TIMER,
        .freq_hz = LCD_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), LCD_TAG, "Configure backlight timer failed");

    const ledc_channel_config_t channel_config = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LCD_LEDC_MODE,
        .channel = LCD_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LCD_LEDC_TIMER,
        .duty = 1023U,
        .hpoint = 0,
    };
    esp_err_t ret = ledc_channel_config(&channel_config);
    if (ret != ESP_OK) {
        const ledc_timer_config_t deconfigure_config = {
            .speed_mode = LCD_LEDC_MODE,
            .timer_num = LCD_LEDC_TIMER,
            .deconfigure = true,
        };
        (void)ledc_timer_config(&deconfigure_config);
        return ret;
    }
    backlight_initialized = true;
    lcd_brightness_percent = 0;
    return ESP_OK;
}

int bsp_display_brightness_get(void)
{
    return lcd_brightness_percent;
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    ESP_RETURN_ON_FALSE(brightness_percent >= 0 && brightness_percent <= 100,
                        ESP_ERR_INVALID_ARG, LCD_TAG, "Brightness must be in range 0..100");
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), LCD_TAG,
                        "Initialize backlight PWM failed");

    const uint32_t duty = 1023U - ((1023U * (uint32_t)brightness_percent) / 100U);
    ESP_RETURN_ON_ERROR(ledc_set_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL, duty), LCD_TAG, "Set backlight duty failed");
    esp_err_t ret = ledc_update_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL);
    if (ret == ESP_OK) {
        lcd_brightness_percent = brightness_percent;
    }
    return ret;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

esp_err_t bsp_display_brightness_deinit(void)
{
    if (!backlight_initialized) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(ledc_stop(LCD_LEDC_MODE, LCD_LEDC_CHANNEL, 1), LCD_TAG, "Stop backlight PWM failed");
    const ledc_timer_config_t timer_config = {
        .speed_mode = LCD_LEDC_MODE,
        .timer_num = LCD_LEDC_TIMER,
        .deconfigure = true,
    };
    esp_err_t ret = ledc_timer_config(&timer_config);
    if (ret == ESP_OK) {
        backlight_initialized = false;
        lcd_brightness_percent = 0;
    }
    return ret;
}

static esp_err_t bsp_enable_dsi_phy_power(void)
{
    if (s_mipi_phy_ldo != NULL) {
        return ESP_OK;
    }

    const esp_ldo_channel_config_t ldo_config = {
        .chan_id = BSP_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = BSP_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_config, &s_mipi_phy_ldo), LCD_TAG, "Acquire MIPI DSI PHY LDO failed");
    ESP_LOGI(LCD_TAG, "MIPI DSI PHY powered by LDO%d at %dmV", BSP_MIPI_DSI_PHY_PWR_LDO_CHAN, BSP_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV);
    return ESP_OK;
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel,
                          esp_lcd_panel_io_handle_t *ret_io)
{
    ESP_RETURN_ON_FALSE(ret_panel && ret_io, ESP_ERR_INVALID_ARG, LCD_TAG, "Invalid display output handle");
    bsp_lcd_handles_t handles = {0};
    ESP_RETURN_ON_ERROR(bsp_display_new_with_handles(config, &handles), LCD_TAG, "Create display failed");
    *ret_panel = handles.panel;
    *ret_io = handles.io;
    return ESP_OK;
}

__attribute__((weak)) esp_err_t bsp_display_new_with_handles(const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles)
{
    ESP_RETURN_ON_FALSE(ret_handles != NULL, ESP_ERR_INVALID_ARG, LCD_TAG, "Invalid display handles");

    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");
    ESP_RETURN_ON_ERROR(bsp_enable_dsi_phy_power(), TAG, "DSI PHY power failed");

    esp_err_t ret = ESP_OK;
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_handle_t panel = NULL;

    /* create MIPI DSI bus first, it will initialize the DSI PHY as well */
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = BSP_LCD_MIPI_DSI_LANE_NUM,
        .phy_clk_src = 0,
        .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &s_mipi_dsi_bus), TAG, "New DSI bus init failed");

    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // MIPI DSI DBI command and parameter width
        .lcd_param_bits = 8, // MIPI DSI DBI command and parameter width
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(s_mipi_dsi_bus, &dbi_config, &io), err, TAG, "New panel IO failed");

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG_CF(LCD_COLOR_FMT_RGB888);
#else
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG_CF(LCD_COLOR_FMT_RGB565);
#endif
#elif CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB888);
#else
    esp_lcd_dpi_panel_config_t dpi_config = EK79007_1024_600_PANEL_60HZ_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
#endif
    dpi_config.num_fbs = (config && config->num_fbs != 0) ? config->num_fbs : CONFIG_BSP_LCD_DPI_BUFFER_NUMS;

    ek79007_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = s_mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
        .bits_per_pixel = 24,
#else
        .bits_per_pixel = 16,
#endif
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = BSP_LCD_COLOR_SPACE,
        .vendor_config = &vendor_config,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ek79007(io, &panel_config, &panel), err, LCD_TAG, "Create EK79007 panel failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(panel), err, LCD_TAG, "Reset EK79007 panel failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(panel), err, LCD_TAG, "Initialize EK79007 panel failed");

    ret_handles->mipi_dsi_bus = s_mipi_dsi_bus;
    ret_handles->io = io;
    ret_handles->panel = panel;
    ret_handles->control = NULL;
    s_display_handles = *ret_handles;
    ESP_LOGI(LCD_TAG, "EK79007 1024x600 MIPI-DSI display initialized");
    return ESP_OK;

err:
    if (panel != NULL) {
        (void)esp_lcd_panel_del(panel);
    }
    if (io != NULL) {
        (void)esp_lcd_panel_io_del(io);
    }
    if (s_mipi_dsi_bus != NULL) {
        (void)esp_lcd_del_dsi_bus(s_mipi_dsi_bus);
        s_mipi_dsi_bus = NULL;
    }
    if (s_mipi_phy_ldo != NULL) {
        esp_err_t cleanup_ret = esp_ldo_release_channel(s_mipi_phy_ldo);
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Release MIPI DSI PHY LDO after display failure failed: %s",
                     esp_err_to_name(cleanup_ret));
        }
        s_mipi_phy_ldo = NULL;
    }
    esp_err_t cleanup_ret = bsp_display_brightness_deinit();
    if (cleanup_ret != ESP_OK) {
        ESP_LOGE(LCD_TAG, "Deinitialize backlight after display failure failed: %s",
                 esp_err_to_name(cleanup_ret));
    }
    return ret;
}

esp_lcd_panel_handle_t bsp_display_get_panel_handle(void)
{
    return s_display_handles.panel;
}

void bsp_display_delete(void)
{
    if (s_display_handles.control) {
        esp_err_t ret = esp_lcd_panel_del(s_display_handles.control);
        if (ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Delete LCD control panel failed: %s", esp_err_to_name(ret));
        }
        s_display_handles.control = NULL;
    }
    if (s_display_handles.panel) {
        esp_err_t ret = esp_lcd_panel_del(s_display_handles.panel);
        if (ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Delete LCD panel failed: %s", esp_err_to_name(ret));
        }
        s_display_handles.panel = NULL;
    }
    if (s_display_handles.io) {
        esp_err_t ret = esp_lcd_panel_io_del(s_display_handles.io);
        if (ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Delete LCD panel IO failed: %s", esp_err_to_name(ret));
        }
        s_display_handles.io = NULL;
    }
    if (s_mipi_dsi_bus) {
        esp_err_t ret = esp_lcd_del_dsi_bus(s_mipi_dsi_bus);
        if (ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Delete MIPI DSI bus failed: %s", esp_err_to_name(ret));
        }
        s_mipi_dsi_bus = NULL;
        s_display_handles.mipi_dsi_bus = NULL;
    }
    if (s_mipi_phy_ldo) {
        esp_err_t ret = esp_ldo_release_channel(s_mipi_phy_ldo);
        if (ret != ESP_OK) {
            ESP_LOGE(LCD_TAG, "Release MIPI DSI PHY LDO failed: %s", esp_err_to_name(ret));
        }
        s_mipi_phy_ldo = NULL;
    }
    esp_err_t ret = bsp_display_brightness_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(LCD_TAG, "Deinitialize backlight failed: %s", esp_err_to_name(ret));
    }
}

static esp_err_t touch_gt911_probe_io(esp_lcd_panel_io_handle_t io)
{
    uint8_t product_id[3] = {0};

    return esp_lcd_panel_io_rx_param(io, BSP_TOUCH_GT911_PRODUCT_ID_REG, product_id, sizeof(product_id));
}

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    ESP_RETURN_ON_FALSE(ret_touch != NULL, ESP_ERR_INVALID_ARG, LCD_TAG, "Invalid touch handle");
    ESP_RETURN_ON_FALSE(BSP_LCD_H_RES > 0 && BSP_LCD_V_RES > 0, ESP_ERR_INVALID_STATE, LCD_TAG,
                        "LCD panel resolution is invalid");
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), LCD_TAG, "Initialize touch I2C bus failed");

    static const uint8_t gt911_addresses[] = {
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP,
    };

    *ret_touch = NULL;
    esp_err_t ret = ESP_FAIL;
    for (size_t i = 0; i < sizeof(gt911_addresses) / sizeof(gt911_addresses[0]); i++) {
        esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        io_config.dev_addr = gt911_addresses[i];
        io_config.scl_speed_hz = BSP_I2C_CLK_SPEED_HZ;

        esp_lcd_panel_io_handle_t io_handle = NULL;
        ret = esp_lcd_new_panel_io_i2c(bsp_i2c_get_handle(), &io_config, &io_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(LCD_TAG, "Create GT911 I2C IO at 0x%02X failed: %s", io_config.dev_addr, esp_err_to_name(ret));
            continue;
        }

        ret = touch_gt911_probe_io(io_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(LCD_TAG, "GT911 probe at I2C address 0x%02X failed: %s", io_config.dev_addr, esp_err_to_name(ret));
            (void)esp_lcd_panel_io_del(io_handle);
            continue;
        }

        s_touch_io = io_handle;
        ESP_ERROR_CHECK(bsp_touch_firmware_update(io_handle));

        esp_lcd_touch_io_gt911_config_t gt911_config = {
            .dev_addr = io_config.dev_addr,
        };
        const esp_lcd_touch_config_t touch_config = {
            .x_max = BSP_LCD_H_RES,
            .y_max = BSP_LCD_V_RES,
            /* The controller was reset before the configuration update. */
            .rst_gpio_num = GPIO_NUM_NC,
            .int_gpio_num = BSP_LCD_TOUCH_INT,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = config ? config->flags.swap_xy : 0,
                .mirror_x = config ? config->flags.mirror_x : 0,
                .mirror_y = config ? config->flags.mirror_y : 0,
            },
            .driver_data = &gt911_config,
        };

        ret = esp_lcd_touch_new_i2c_gt911(io_handle, &touch_config, ret_touch);
        if (ret == ESP_OK) {
            ESP_LOGI(LCD_TAG, "GT911 detected at I2C address 0x%02X", io_config.dev_addr);
            s_touch = *ret_touch;
            return ESP_OK;
        }

        ESP_LOGW(LCD_TAG, "GT911 not found at I2C address 0x%02X: %s", io_config.dev_addr, esp_err_to_name(ret));
        s_touch_io = NULL;
        (void)esp_lcd_panel_io_del(io_handle);
    }
    return ret;
}

void bsp_touch_delete(void)
{
    if (s_touch != NULL) {
        (void)esp_lcd_touch_del(s_touch);
        s_touch = NULL;
    }
    if (s_touch_io != NULL) {
        (void)esp_lcd_panel_io_del(s_touch_io);
        s_touch_io = NULL;
    }
}

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *config)
{
    const bsp_display_config_t hw_config = {
        .dsi_bus = {
            .phy_clk_src = 0,
            .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
        },
        .num_fbs = esp_lv_adapter_get_required_frame_buffer_count(config->tear_avoid_mode, config->rotation),
    };
    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new_with_handles(&hw_config, &s_display_handles));

    esp_lv_adapter_display_config_t display_config = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(s_display_handles.panel, s_display_handles.io, BSP_LCD_H_RES, BSP_LCD_V_RES, config->rotation);
    display_config.tear_avoid_mode = config->tear_avoid_mode;
    return esp_lv_adapter_register_display(&display_config);
}

static lv_indev_t *bsp_display_touch_init(lv_display_t *display, const bsp_display_cfg_t *config)
{
    const bsp_touch_config_t touch_config = {
        .flags = {
            .swap_xy = config->touch_flags.swap_xy,
            .mirror_x = config->touch_flags.mirror_x,
            .mirror_y = config->touch_flags.mirror_y,
        },
    };
    if (bsp_touch_new(&touch_config, &s_touch) != ESP_OK) {
        return NULL;
    }

    const esp_lv_adapter_touch_config_t adapter_touch_config = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, s_touch);
    return esp_lv_adapter_register_touch(&adapter_touch_config);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t config = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    return bsp_display_start_with_config(&config);
}

lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *config)
{
    if (config == NULL) {
        ESP_LOGE(LCD_TAG, "Display configuration is NULL");
        return NULL;
    }

    esp_err_t ret = esp_lv_adapter_init(&config->lv_adapter_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(LCD_TAG, "Initialize LVGL adapter failed: %s", esp_err_to_name(ret));
        return NULL;
    }

    s_display = bsp_display_lcd_init(config);
    if (s_display == NULL) {
        ESP_LOGE(LCD_TAG, "Register display with LVGL adapter failed");
        goto err;
    }

    s_touch_indev = bsp_display_touch_init(s_display, config);
    if (s_touch_indev == NULL) {
        ESP_LOGW(LCD_TAG, "Touch unavailable; continuing without LVGL input device");
        bsp_touch_delete();
    }

    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) {
        ESP_LOGE(LCD_TAG, "Start LVGL adapter failed: %s", esp_err_to_name(ret));
        goto err;
    }
    return s_display;

err:
    if (s_touch_indev != NULL) {
        esp_lv_adapter_unregister_touch(s_touch_indev);
        s_touch_indev = NULL;
    }
    bsp_touch_delete();
    if (s_display != NULL) {
        esp_lv_adapter_unregister_display(s_display);
        s_display = NULL;
    }
    bsp_display_delete();
    esp_lv_adapter_deinit();
    return NULL;
}

void bsp_display_stop(lv_display_t *display)
{
    if (s_touch_indev != NULL) {
        esp_lv_adapter_unregister_touch(s_touch_indev);
        s_touch_indev = NULL;
    }
    lv_display_t *display_to_remove = display ? display : s_display;
    if (display_to_remove) {
        esp_lv_adapter_unregister_display(display_to_remove);
    }
    esp_lv_adapter_deinit();
    bsp_touch_delete();
    bsp_display_delete();
    s_display = NULL;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return s_touch_indev;
}

void bsp_display_rotate(lv_display_t *display, lv_disp_rotation_t rotation)
{
    lv_disp_set_rotation(display, rotation);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

void bsp_display_unlock(void)
{
    esp_lv_adapter_unlock();
}

esp_err_t bsp_display_set_dummy_draw(bool enable)
{
    if (s_display == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lv_adapter_set_dummy_draw(s_display, enable);
}

void *bsp_display_get_free_frame_buffer(void)
{
    if (s_display == NULL) {
        return NULL;
    }
    return esp_lv_adapter_dummy_draw_get_free_buf(s_display);
}

esp_err_t bsp_display_flush_frame_buffer(void *frame_buffer)
{
    if (s_display == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (frame_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_lv_adapter_dummy_draw_flush_buf(s_display, frame_buffer);
}
#endif

static void usb_lib_task(void *arg)
{
    (void)arg;
    while (!atomic_load(&usb_host_stop_requested)) {
        uint32_t event_flags = 0;
        esp_err_t ret = usb_host_lib_handle_events(pdMS_TO_TICKS(100), &event_flags);
        if (ret == ESP_ERR_TIMEOUT) {
            continue;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "USB Host event handling failed: %s", esp_err_to_name(ret));
            break;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ret = usb_host_device_free_all();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "USB Host device cleanup failed: %s", esp_err_to_name(ret));
            }
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
        }
    }
    usb_host_task = NULL;
    if (usb_host_done_sem != NULL) {
        xSemaphoreGive(usb_host_done_sem);
    }
    vTaskDelete(NULL);
}

esp_err_t bsp_usb_host_start(bsp_usb_host_power_mode_t mode, bool limit_500mA)
{
    (void)mode;
    (void)limit_500mA;
    ESP_RETURN_ON_FALSE(usb_host_task == NULL, ESP_ERR_INVALID_STATE, TAG,
                        "USB Host is already running");
    if (usb_host_done_sem == NULL) {
        usb_host_done_sem = xSemaphoreCreateBinary();
        ESP_RETURN_ON_FALSE(usb_host_done_sem != NULL, ESP_ERR_NO_MEM, TAG,
                            "Create USB Host completion semaphore failed");
    }
    while (xSemaphoreTake(usb_host_done_sem, 0) == pdTRUE) {
    }
    atomic_store(&usb_host_stop_requested, false);
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        vSemaphoreDelete(usb_host_done_sem);
        usb_host_done_sem = NULL;
        return ret;
    }
    if (xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, &usb_host_task) != pdTRUE) {
        (void)usb_host_uninstall();
        vSemaphoreDelete(usb_host_done_sem);
        usb_host_done_sem = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t bsp_usb_host_stop(void)
{
    if (usb_host_task != NULL) {
        atomic_store(&usb_host_stop_requested, true);
        ESP_RETURN_ON_FALSE(
            usb_host_done_sem != NULL &&
            xSemaphoreTake(usb_host_done_sem, pdMS_TO_TICKS(1000)) == pdTRUE,
            ESP_ERR_TIMEOUT, TAG, "Timed out waiting for USB Host task to stop"
        );
    }

    esp_err_t ret = usb_host_uninstall();
    if (ret != ESP_OK) {
        atomic_store(&usb_host_stop_requested, false);
        if (usb_host_done_sem != NULL &&
            xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, &usb_host_task) != pdTRUE) {
            ESP_LOGE(TAG, "Restart USB Host task after uninstall failure failed");
        }
        return ret;
    }
    if (usb_host_done_sem != NULL) {
        vSemaphoreDelete(usb_host_done_sem);
        usb_host_done_sem = NULL;
    }
    return ESP_OK;
}

esp_err_t bsp_camera_start(const bsp_camera_cfg_t *cfg)
{
    (void)cfg;
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());
    const esp_video_init_csi_config_t csi_config = {
        .sccb_config = {
            .init_sccb = false,
            .i2c_handle = i2c_handle,
            .freq = BSP_I2C_CLK_SPEED_HZ,
        },
        .reset_pin = BSP_CAMERA_RST,
        .pwdn_pin = GPIO_NUM_NC,
    };
    const esp_video_init_config_t camera_config = {
        .csi = &csi_config,
    };
    return esp_video_init(&camera_config);
}
