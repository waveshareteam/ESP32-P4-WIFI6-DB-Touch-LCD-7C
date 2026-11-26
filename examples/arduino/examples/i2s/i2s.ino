/*
 * ES8389 microphone-to-speaker loopback.
 *
 * Based on ESP_I2S/examples/ES8388_loopback and the ES8389 implementation in
 * esp_codec_dev. The codec setup matches this board's ESP-IDF BSP.
 */

#include <Arduino.h>
#include <ESP_I2S.h>
#include <Wire.h>

#include "ES8389.h"

constexpr uint32_t i2c_frequency_hz = 400000;
constexpr uint32_t sample_rate_hz = 48000;
constexpr size_t audio_buffer_size = 1024;
constexpr int i2c_sda_pin = 7;
constexpr int i2c_scl_pin = 8;
constexpr int i2s_dout_pin = 9;
constexpr int i2s_lrclk_pin = 10;
constexpr int i2s_din_pin = 11;
constexpr int i2s_bclk_pin = 12;
constexpr int pa_power_pin = 53;

I2SClass i2s;
ES8389 codec;
int16_t audio_buffer[audio_buffer_size / sizeof(int16_t)];
uint32_t last_audio_log_ms = 0;

void scan_i2c_bus()
{
    uint8_t device_count = 0;
    for (uint8_t address = 1; address < 0x7F; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("I2C device found at 0x%02X\n", address);
            ++device_count;
        }
    }
    Serial.printf("I2C scan complete: %u device(s)\n", device_count);
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("Starting ES8389 I2S loopback");

    if (!Wire.begin(i2c_sda_pin, i2c_scl_pin, i2c_frequency_hz)) {
        Serial.println("Failed to initialize I2C");
        return;
    }
    scan_i2c_bus();

    i2s.setPins(i2s_bclk_pin, i2s_lrclk_pin, i2s_dout_pin, i2s_din_pin);
    if (!i2s.begin(I2S_MODE_STD, sample_rate_hz, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
        Serial.println("Failed to initialize I2S");
        return;
    }

    if (!codec.begin(Wire, pa_power_pin)) {
        Serial.println("Failed to initialize ES8389 at I2C address 0x10");
        i2s.end();
        return;
    }

    Serial.println("ES8389 ready: 48000 Hz, 16-bit stereo, differential microphones");
    Serial.println("Microphone audio is now sent to the speaker. Avoid acoustic feedback.");
}

void loop()
{
    size_t bytes_read = i2s.readBytes(reinterpret_cast<char *>(audio_buffer), sizeof(audio_buffer));
    if (bytes_read == 0) {
        return;
    }

    int32_t left_peak = 0;
    int32_t right_peak = 0;
    const size_t sample_count = bytes_read / sizeof(audio_buffer[0]);
    for (size_t index = 0; index + 1 < sample_count; index += 2) {
        const int32_t left_sample = audio_buffer[index];
        const int32_t right_sample = audio_buffer[index + 1];
        const int32_t left_level = left_sample < 0 ? -left_sample : left_sample;
        const int32_t right_level = right_sample < 0 ? -right_sample : right_sample;

        if (left_level > left_peak) {
            left_peak = left_level;
        }
        if (right_level > right_peak) {
            right_peak = right_level;
        }

        // Mix both microphones to mono and send the result to both DAC slots.
        const int16_t mono_sample = static_cast<int16_t>((left_sample + right_sample) / 2);
        audio_buffer[index] = mono_sample;
        audio_buffer[index + 1] = mono_sample;
    }

    const uint32_t now_ms = millis();
    if (now_ms - last_audio_log_ms >= 1000) {
        Serial.printf("I2S input peak: left=%ld right=%ld\n", static_cast<long>(left_peak), static_cast<long>(right_peak));
        last_audio_log_ms = now_ms;
    }

    size_t bytes_written = i2s.write(reinterpret_cast<const uint8_t *>(audio_buffer), bytes_read);
    if (bytes_written != bytes_read) {
        Serial.printf("I2S short write: %u/%u bytes\n", static_cast<unsigned>(bytes_written), static_cast<unsigned>(bytes_read));
    }
}
