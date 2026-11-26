#include <Arduino.h>

#ifndef BOOT_PIN
#define BOOT_PIN 35
#endif

static constexpr uint32_t debounce_ms = 30;
static constexpr uint32_t long_press_ms = 1000;

static bool last_raw_pressed = false;
static bool stable_pressed = false;
static uint32_t last_change_ms = 0;
static uint32_t press_start_ms = 0;
static bool long_press_reported = false;

void setup()
{
    Serial.begin(115200);
    delay(500);

    pinMode(BOOT_PIN, INPUT_PULLUP);
    Serial.printf("BOOT button example ready on GPIO%u (active LOW)\n", BOOT_PIN);
    Serial.println("Press Key2 (BOOT) for short/long press events");
}

void loop()
{
    const uint32_t now_ms = millis();
    const bool raw_pressed = (digitalRead(BOOT_PIN) == LOW);

    if (raw_pressed != last_raw_pressed) {
        last_raw_pressed = raw_pressed;
        last_change_ms = now_ms;
    }

    if ((now_ms - last_change_ms) >= debounce_ms && raw_pressed != stable_pressed) {
        stable_pressed = raw_pressed;
        if (stable_pressed) {
            press_start_ms = now_ms;
            long_press_reported = false;
            Serial.println("BOOT button pressed");
        } else {
            const uint32_t duration_ms = now_ms - press_start_ms;
            if (!long_press_reported) {
                Serial.printf("BOOT button short press (%lu ms)\n", static_cast<unsigned long>(duration_ms));
            }
            Serial.println("BOOT button released");
        }
    }

    if (stable_pressed && !long_press_reported && (now_ms - press_start_ms) >= long_press_ms) {
        long_press_reported = true;
        Serial.println("BOOT button long press (>= 1000 ms)");
    }

    delay(5);
}
