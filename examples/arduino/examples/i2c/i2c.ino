#include <Arduino.h>
#include <Wire.h>

static constexpr int i2c_sda_pin = 7;
static constexpr int i2c_scl_pin = 8;

void scan_i2c_bus()
{
    uint8_t device_count = 0;

    Serial.println("Scanning I2C bus...");
    for (uint8_t address = 1; address < 0x7f; ++address) {
        Wire.beginTransmission(address);
        const uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("Device found at 0x%02X\n", address);
            ++device_count;
        } else if (error != 2) {
            Serial.printf("Error %u at 0x%02X\n", error, address);
        }
    }
    Serial.printf("Scan complete: %u device(s) found\n", device_count);
}

void setup()
{
    Serial.begin(115200);
    Wire.begin(i2c_sda_pin, i2c_scl_pin);
}

void loop()
{
    scan_i2c_bus();
    delay(5000);
}
