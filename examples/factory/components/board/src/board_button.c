/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_board_manager_includes.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "board_button.h"

#if defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT) && __has_include("dev_button.h")
#include "dev_button.h"
#define BOARD_HAS_DEV_BUTTON    (1)
#else
#define BOARD_HAS_DEV_BUTTON    (0)
#endif

static const char *TAG = "board_button";

#define BOARD_BUTTON_BOOT_DEV_NAME      "boot_button"
#define BOARD_BUTTON_BOOT_GPIO          (GPIO_NUM_35)
#define BOARD_BUTTON_BOOT_ACTIVE_LEVEL  (0)

static bool s_initialized = false;
static bool s_using_bmgr = false;
static bool s_boot_cb_registered = false;
static bool s_boot_long_cb_registered = false;
static button_handle_t s_boot_btn = NULL;
static portMUX_TYPE s_boot_cb_lock = portMUX_INITIALIZER_UNLOCKED;
static board_button_click_cb_t s_boot_click_cb = NULL;
static void *s_boot_click_arg = NULL;

static void boot_button_iot_cb(void *button_handle, void *usr_data)
{
    (void)button_handle;
    (void)usr_data;
    portENTER_CRITICAL(&s_boot_cb_lock);
    board_button_click_cb_t cb = s_boot_click_cb;
    void *arg = s_boot_click_arg;
    portEXIT_CRITICAL(&s_boot_cb_lock);
    if (cb) {
        cb(arg);
    } else {
        ESP_LOGI(TAG, "BOOT button single click");
    }
}

static void boot_button_long_press_cb(void *button_handle, void *usr_data)
{
    (void)button_handle;
    (void)usr_data;
    ESP_LOGI(TAG, "BOOT button long press");
}

static void boot_button_register_default_cbs(void)
{
    if (iot_button_register_cb(s_boot_btn, BUTTON_SINGLE_CLICK, NULL, boot_button_iot_cb, NULL) == ESP_OK) {
        s_boot_cb_registered = true;
    } else {
        ESP_LOGW(TAG, "Register BOOT single-click callback failed");
    }
    if (iot_button_register_cb(s_boot_btn, BUTTON_LONG_PRESS_START, NULL, boot_button_long_press_cb, NULL) == ESP_OK) {
        s_boot_long_cb_registered = true;
    } else {
        ESP_LOGW(TAG, "Register BOOT long-press callback failed");
    }
}

esp_err_t board_button_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

#if BOARD_HAS_DEV_BUTTON
    if (esp_board_manager_check_name(BOARD_BUTTON_BOOT_DEV_NAME)) {
        ESP_RETURN_ON_ERROR(esp_board_manager_init_device_by_name(BOARD_BUTTON_BOOT_DEV_NAME),
                            TAG, "Initialize boot_button device failed");
        dev_button_handles_t *handles = NULL;
        esp_err_t ret = esp_board_manager_get_device_handle(BOARD_BUTTON_BOOT_DEV_NAME, (void **)&handles);
        if (ret != ESP_OK || !handles || handles->num_buttons == 0 || !handles->button_handles[0]) {
            (void)esp_board_manager_deinit_device_by_name(BOARD_BUTTON_BOOT_DEV_NAME);
            return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
        }
        s_boot_btn = handles->button_handles[0];
        boot_button_register_default_cbs();
        s_using_bmgr = true;
        s_initialized = true;
        return ESP_OK;
    }
#endif

    const button_config_t btn_cfg = {0};
    const button_gpio_config_t gpio_cfg = {
        .gpio_num = BOARD_BUTTON_BOOT_GPIO,
        .active_level = BOARD_BUTTON_BOOT_ACTIVE_LEVEL,
    };
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_boot_btn),
                        TAG, "Create BOOT button device failed");
    boot_button_register_default_cbs();
    s_using_bmgr = false;
    s_initialized = true;
    return ESP_OK;
}

esp_err_t board_button_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    if (s_boot_cb_registered && s_boot_btn) {
        (void)iot_button_unregister_cb(s_boot_btn, BUTTON_SINGLE_CLICK, NULL);
        s_boot_cb_registered = false;
    }
    if (s_boot_long_cb_registered && s_boot_btn) {
        (void)iot_button_unregister_cb(s_boot_btn, BUTTON_LONG_PRESS_START, NULL);
        s_boot_long_cb_registered = false;
    }

    portENTER_CRITICAL(&s_boot_cb_lock);
    s_boot_click_cb = NULL;
    s_boot_click_arg = NULL;
    portEXIT_CRITICAL(&s_boot_cb_lock);

    esp_err_t ret = ESP_OK;
    if (s_using_bmgr) {
        ret = esp_board_manager_deinit_device_by_name(BOARD_BUTTON_BOOT_DEV_NAME);
    } else if (s_boot_btn) {
        ret = iot_button_delete(s_boot_btn);
    }

    s_boot_btn = NULL;
    s_using_bmgr = false;
    s_initialized = false;
    return ret;
}

esp_err_t board_button_is_boot_pressed(bool *pressed)
{
    ESP_RETURN_ON_FALSE(pressed, ESP_ERR_INVALID_ARG, TAG, "pressed is NULL");
    ESP_RETURN_ON_FALSE(s_initialized && s_boot_btn, ESP_ERR_INVALID_STATE, TAG, "Button is not initialized");

    *pressed = (iot_button_get_key_level(s_boot_btn) != 0);
    return ESP_OK;
}

esp_err_t board_button_register_boot_click_cb(board_button_click_cb_t cb, void *arg)
{
    if (!cb) {
        portENTER_CRITICAL(&s_boot_cb_lock);
        s_boot_click_cb = NULL;
        s_boot_click_arg = NULL;
        portEXIT_CRITICAL(&s_boot_cb_lock);
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(s_initialized && s_boot_btn, ESP_ERR_INVALID_STATE, TAG, "Button is not initialized");

    portENTER_CRITICAL(&s_boot_cb_lock);
    s_boot_click_arg = arg;
    s_boot_click_cb = cb;
    portEXIT_CRITICAL(&s_boot_cb_lock);

    if (!s_boot_cb_registered) {
        esp_err_t ret = iot_button_register_cb(
            s_boot_btn, BUTTON_SINGLE_CLICK, NULL, boot_button_iot_cb, NULL);
        if (ret != ESP_OK) {
            portENTER_CRITICAL(&s_boot_cb_lock);
            s_boot_click_cb = NULL;
            s_boot_click_arg = NULL;
            portEXIT_CRITICAL(&s_boot_cb_lock);
            return ret;
        }
        s_boot_cb_registered = true;
    }
    return ESP_OK;
}
