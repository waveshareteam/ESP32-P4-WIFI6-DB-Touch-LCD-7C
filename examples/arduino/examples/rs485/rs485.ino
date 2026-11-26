#include "hal/uart_types.h"
#include <Arduino.h>

static constexpr int rs485_tx_pin = 26;
static constexpr int rs485_rx_pin = 27;
static constexpr int rs485_dir_pin = 34;

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, rs485_rx_pin, rs485_tx_pin);

    if (!Serial1.setPins(-1, -1, -1, rs485_dir_pin)) {
        Serial.println("RS485 direction pin configuration failed");
        return;
    }
    if (!Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
        Serial.println("RS485 half-duplex mode configuration failed");
    }
}

void loop()
{
    while (Serial1.available()) {
        Serial.write(Serial1.read());
    }
    while (Serial.available()) {
        Serial1.write(Serial.read());
    }
}
