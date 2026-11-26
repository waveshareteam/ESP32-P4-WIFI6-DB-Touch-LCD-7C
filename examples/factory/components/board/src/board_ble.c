/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_ble.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ENABLED) && __has_include("esp_hosted_bt_host_stack.h")
#include "esp_check.h"
#include "esp_hosted_bt_host_stack.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "board_ble";

static bool s_ble_initialized = false;
static bool s_hosted_bt_bound = false;

static void board_ble_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGW(TAG, "Ensure BLE address failed: %d", rc);
    }
}

static void board_ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE reset, reason=%d", reason);
}

static void board_ble_host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t board_ble_init(void)
{
    if (s_ble_initialized) {
        return ESP_OK;
    }

    if (!s_hosted_bt_bound) {
        esp_hosted_bt_host_stack_cfg_t bt_cfg = ESP_HOSTED_BT_HOST_STACK_CONFIG_DEFAULT();
        ESP_RETURN_ON_ERROR(esp_hosted_bt_host_stack_setup(&bt_cfg), TAG, "Setup hosted BT stack failed");
        s_hosted_bt_bound = true;
    }

    ESP_RETURN_ON_ERROR(nimble_port_init(), TAG, "Initialize NimBLE port failed");

    ble_hs_cfg.sync_cb = board_ble_on_sync;
    ble_hs_cfg.reset_cb = board_ble_on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    nimble_port_freertos_init(board_ble_host_task);

    s_ble_initialized = true;
    ESP_LOGI(TAG, "BLE stack initialized");
    return ESP_OK;
}

esp_err_t board_ble_deinit(void)
{
    if (!s_ble_initialized) {
        return ESP_OK;
    }

    int rc = nimble_port_stop();
    if (rc != 0) {
        ESP_LOGW(TAG, "Stop NimBLE port returned %d", rc);
    }
    esp_err_t ret = nimble_port_deinit();
    if (ret == ESP_OK) {
        s_ble_initialized = false;
    }
    return ret;
}

bool board_ble_is_initialized(void)
{
    return s_ble_initialized;
}
#else
esp_err_t board_ble_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t board_ble_deinit(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

bool board_ble_is_initialized(void)
{
    return false;
}
#endif
