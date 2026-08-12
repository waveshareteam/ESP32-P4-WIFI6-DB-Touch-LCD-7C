/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "uart_loopback";
static const uart_port_t UART_PORT = CONFIG_EXAMPLE_UART_PORT_NUM;

#define UART_TX_QUEUE_LENGTH 8
#define UART_RX_RING_SIZE    4096
#define UART_RX_CHUNK_SIZE   256

#if defined(CONFIG_EXAMPLE_UART_RS485_ENABLE)
#define CONFIG_EXAMPLE_UART_TX_GPIO         GPIO_NUM_26
#define CONFIG_EXAMPLE_UART_RX_GPIO         GPIO_NUM_27
#define CONFIG_EXAMPLE_UART_RS485_DIR_GPIO  GPIO_NUM_34
#else
#define CONFIG_EXAMPLE_UART_TX_GPIO         GPIO_NUM_28
#define CONFIG_EXAMPLE_UART_RX_GPIO         GPIO_NUM_29
#define CONFIG_EXAMPLE_UART_RS485_DIR_GPIO  UART_PIN_NO_CHANGE
#endif

typedef struct {
    size_t length;
    uint8_t data[UART_RX_CHUNK_SIZE];
} uart_tx_item_t;

static QueueHandle_t s_tx_queue;
static RingbufHandle_t s_rx_ring;

static bool uart_queue_tx(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0 || length > UART_RX_CHUNK_SIZE) {
        return false;
    }

    uart_tx_item_t item = {
        .length = length,
    };
    memcpy(item.data, data, length);

    if (xQueueSend(s_tx_queue, &item, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "TX queue full, loopback data dropped");
        return false;
    }
    return true;
}

static void uart_tx_worker_task(void *arg)
{
    (void)arg;
    uart_tx_item_t item;

    while (true) {
        if (xQueueReceive(s_tx_queue, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        int written = uart_write_bytes(UART_PORT, item.data, item.length);
        if (written != (int)item.length) {
            ESP_LOGE(TAG, "uart_write_bytes failed or incomplete: %d/%u",
                     written, (unsigned)item.length);
        }

        esp_err_t err = uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "uart_wait_tx_done failed: %s", esp_err_to_name(err));
        }
    }
}

static void uart_rx_task(void *arg)
{
    (void)arg;
    uint8_t rx_buffer[UART_RX_CHUNK_SIZE];

    while (true) {
        int len = uart_read_bytes(
            UART_PORT, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(100)
        );
        if (len > 0) {
            if (xRingbufferSend(s_rx_ring, rx_buffer, len, 0) != pdTRUE) {
                ESP_LOGW(TAG, "RX ring buffer full, dropped %d bytes", len);
            }
            // Echo received data back through the TX queue.
            uart_queue_tx(rx_buffer, len);
        }
    }
}

static void uart_rx_log_task(void *arg)
{
    (void)arg;

    while (true) {
        size_t len = 0;
        uint8_t *data = (uint8_t *)xRingbufferReceive(s_rx_ring, &len, portMAX_DELAY);
        if (data == NULL) {
            continue;
        }

        ESP_LOGI(TAG, "received %u bytes: %.*s", (unsigned)len, (int)len, (char *)data);
        vRingbufferReturnItem(s_rx_ring, data);
    }
}

void app_main(void)
{
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_EXAMPLE_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    printf("\n");
    printf("========================================\n");
    printf(" UART Loopback Demo\n");
    printf("========================================\n");
    printf("Connect TX GPIO %d to RX GPIO %d.\n", CONFIG_EXAMPLE_UART_TX_GPIO, CONFIG_EXAMPLE_UART_RX_GPIO);
    printf("Baud rate: %d\n", CONFIG_EXAMPLE_UART_BAUD_RATE);
    printf("\n");

#if CONFIG_EXAMPLE_UART_RS485_ENABLE
    // Keep the RS485 transceiver in receive mode until UART takes over RTS.
    gpio_set_direction(CONFIG_EXAMPLE_UART_RS485_DIR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_EXAMPLE_UART_RS485_DIR_GPIO, 0);
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                 CONFIG_EXAMPLE_UART_TX_GPIO,
                                 CONFIG_EXAMPLE_UART_RX_GPIO,
                                 CONFIG_EXAMPLE_UART_RS485_DIR_GPIO,
                                 UART_PIN_NO_CHANGE));
#if CONFIG_EXAMPLE_UART_RS485_ENABLE
    ESP_ERROR_CHECK(uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX));
#endif

    s_tx_queue = xQueueCreate(UART_TX_QUEUE_LENGTH, sizeof(uart_tx_item_t));
    s_rx_ring = xRingbufferCreate(UART_RX_RING_SIZE, RINGBUF_TYPE_BYTEBUF);
    ESP_ERROR_CHECK(s_tx_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(s_rx_ring != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    xTaskCreate(uart_tx_worker_task, "uart_tx_worker", 3072, NULL, 5, NULL);
    xTaskCreate(uart_rx_task, "uart_rx", 3072, NULL, 5, NULL);
    xTaskCreate(uart_rx_log_task, "uart_rx_log", 3072, NULL, 5, NULL);
}
