#include "driver/twai.h"
#include <Arduino.h>


static constexpr gpio_num_t twai_tx_pin = GPIO_NUM_22;
static constexpr gpio_num_t twai_rx_pin = GPIO_NUM_21;
static bool twai_started = false;

void setup()
{
    Serial.begin(115200);

    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(twai_tx_pin, twai_rx_pin, TWAI_MODE_LISTEN_ONLY);
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&general_config, &timing_config, &filter_config) != ESP_OK) {
        Serial.println("TWAI driver installation failed");
        return;
    }
    if (twai_start() != ESP_OK) {
        Serial.println("TWAI driver start failed");
        return;
    }
    twai_started = true;
    Serial.println("TWAI receiver started at 500 kbit/s");
}

void loop()
{
    if (!twai_started) {
        delay(1000);
        return;
    }

    twai_message_t message = {};
    if (twai_receive(&message, pdMS_TO_TICKS(1000)) != ESP_OK) {
        return;
    }

    Serial.printf("ID: 0x%08lX DLC: %u Data:", (unsigned long)message.identifier, message.data_length_code);
    if (!message.rtr) {
        for (uint8_t index = 0; index < message.data_length_code; ++index) {
            Serial.printf(" %02X", message.data[index]);
        }
    }
    Serial.println();
}
