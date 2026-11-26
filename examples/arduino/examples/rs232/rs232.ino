#include <Arduino.h>

static constexpr int rs232_tx_pin = 28;
static constexpr int rs232_rx_pin = 29;

void setup()
{
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, rs232_rx_pin, rs232_tx_pin);
    Serial.println("RS232 serial bridge started");
}

void loop()
{
    while (Serial2.available()) {
        Serial.write(Serial2.read());
    }
    while (Serial.available()) {
        Serial2.write(Serial.read());
    }
}
