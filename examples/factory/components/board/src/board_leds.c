/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_leds.h"

#include "driver/gpio.h"
#include "esp_board_manager_includes.h"
#include "esp_check.h"

static const char *TAG = "board_leds";

static const char *const LED_DEVICE_NAMES[BOARD_USER_LED_COUNT] = {"user_led_1", "user_led_2"};
static bool s_led_ready[BOARD_USER_LED_COUNT];
static bool s_led_on[BOARD_USER_LED_COUNT];

static esp_err_t board_user_led_prepare(int index, periph_gpio_handle_t **gpio, dev_gpio_ctrl_config_t **cfg)
{
    ESP_RETURN_ON_FALSE(index >= 0 && index < BOARD_USER_LED_COUNT, ESP_ERR_INVALID_ARG, TAG, "Invalid LED index");
    const char *name = LED_DEVICE_NAMES[index];
    if (!s_led_ready[index]) {
        ESP_RETURN_ON_ERROR(esp_board_manager_init_device_by_name(name), TAG, "Initialize %s failed", name);
    }
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle(name, (void **)gpio), TAG, "Get %s handle failed", name);
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_config(name, (void **)cfg), TAG, "Get %s config failed", name);
    ESP_RETURN_ON_FALSE(*gpio && *cfg, ESP_ERR_INVALID_STATE, TAG, "%s is not available", name);
    return ESP_OK;
}

esp_err_t board_user_led_set(int index, bool on)
{
    periph_gpio_handle_t *gpio = NULL;
    dev_gpio_ctrl_config_t *cfg = NULL;
    ESP_RETURN_ON_ERROR(board_user_led_prepare(index, &gpio, &cfg), TAG, "Prepare LED failed");

    const uint32_t level = on ? cfg->active_level : !cfg->active_level;
    ESP_RETURN_ON_ERROR(gpio_set_level(gpio->gpio_num, level), TAG, "Set LED level failed");
    s_led_ready[index] = true;
    s_led_on[index] = on;
    return ESP_OK;
}

esp_err_t board_user_led_get(int index, bool *on)
{
    ESP_RETURN_ON_FALSE(on && index >= 0 && index < BOARD_USER_LED_COUNT, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");
    *on = s_led_on[index];
    return ESP_OK;
}
