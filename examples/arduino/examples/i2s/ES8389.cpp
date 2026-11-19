#include "ES8389.h"

namespace {

// ESP_CODEC_DEV uses 0x20 for this codec. Arduino Wire uses its 7-bit form.
constexpr uint8_t es8389_addr = 0x10;

struct reg_value_t {
  uint8_t reg;
  uint8_t value;
};

// Derived from es8389_open() in esp_codec_dev's ES8389 driver.
constexpr reg_value_t init_sequence[] = {
  {0xF3, 0x00}, {0x00, 0x7E}, {0xF3, 0x38}, {0x24, 0x64}, {0x25, 0x04},
  {0x45, 0x03}, {0x60, 0x2A}, {0x61, 0xC9}, {0x62, 0x4F}, {0x63, 0x06},
  {0x6B, 0x00}, {0x6D, 0x16}, {0x6E, 0xAA}, {0x6F, 0x66}, {0x70, 0x99},
  {0x23, 0x00}, {0x72, 0x00}, {0x73, 0x00}, {0x10, 0xC4}, {0x01, 0x08},
  {0xF1, 0x00}, {0x12, 0x01}, {0x13, 0x01}, {0x14, 0x01}, {0x15, 0x01},
  {0x16, 0x35}, {0x17, 0x09}, {0x18, 0x91}, {0x19, 0x28}, {0x1A, 0x01},
  {0x1B, 0x01}, {0x1C, 0x11}, {0x2A, 0x00}, {0x20, 0x00}, {0x40, 0x00},
  {0xF0, 0x01}, {0x02, 0x00}, {0x04, 0x00}, {0x05, 0x10}, {0x06, 0x00},
  {0x07, 0xC0}, {0x08, 0x00}, {0x09, 0xC0}, {0x0A, 0x80}, {0x0B, 0x04},
  {0x0C, 0x01}, {0x0D, 0x00}, {0x0F, 0x10}, {0x21, 0x1F}, {0x22, 0x7F},
  {0x2F, 0xC0}, {0x30, 0xF4}, {0x31, 0x00}, {0x44, 0x00}, {0x41, 0x7F},
  {0x42, 0x7F}, {0x43, 0x10}, {0x49, 0x0F}, {0x4C, 0xC0}, {0x00, 0x00},
  {0x03, 0xC1}, {0x00, 0x01}, {0x4D, 0x00}, {0x26, 0xBF}, {0x27, 0xBF},
  {0x28, 0xBF}, {0x46, 0xBF}, {0x47, 0xBF}, {0x48, 0xBE},
};

// This is the coefficient selected by es8389_config_sample() for 48000 Hz,
// 16-bit stereo with the codec's internal clock configuration.
constexpr reg_value_t sample_rate_sequence[] = {
  {0x04, 0x00}, {0x05, 0x51}, {0x06, 0x04}, {0x07, 0xC0}, {0x08, 0x10},
  {0x09, 0xC1}, {0x0A, 0x80}, {0x0F, 0x00}, {0x11, 0x00}, {0x21, 0x1F},
  {0x22, 0x7F}, {0x26, 0xBF}, {0x30, 0xC0}, {0x41, 0x7F}, {0x42, 0x7F},
  {0x43, 0x00}, {0xF0, 0x12}, {0xF1, 0x00}, {0x16, 0x35}, {0x18, 0x91},
  {0x19, 0x28},
};

}  // namespace

bool ES8389::begin(TwoWire &i2c, uint8_t pa_pin) {
  _i2c = &i2c;

  _i2c->beginTransmission(es8389_addr);
  uint8_t i2c_error = _i2c->endTransmission();
  if (i2c_error != 0) {
    Serial.printf("ES8389 I2C probe failed at 0x%02X, error %u\n", es8389_addr, i2c_error);
    return false;
  }

  pinMode(pa_pin, OUTPUT);
  digitalWrite(pa_pin, LOW);

  for (const auto &entry : init_sequence) {
    if (!write_reg(entry.reg, entry.value)) {
      Serial.printf("ES8389 initialization write failed at register 0x%02X\n", entry.reg);
      return false;
    }
  }
  for (const auto &entry : sample_rate_sequence) {
    if (!write_reg(entry.reg, entry.value)) {
      Serial.printf("ES8389 sample rate write failed at register 0x%02X\n", entry.reg);
      return false;
    }
  }

  // Match the codec serial word length to the 16-bit I2S slot configuration.
  if (!update_bits(0x20, 0xE0, 0x60) || !update_bits(0x40, 0xE0, 0x60)) {
    Serial.println("ES8389 I2S word length configuration failed");
    return false;
  }

  // Board BSP: codec slave mode, internal clock, and two differential mics.
  if (!update_bits(0x01, 0x01, 0x00) || !update_bits(0x02, 0xC0, 0x40) ||
      !update_bits(0x02, 0x03, 0x00) || !write_reg(0x72, 0x1A) ||
      !write_reg(0x73, 0x1A) || !write_reg(0x4D, 0x00) ||
      !update_bits(0x69, 0x20, 0x20) || !write_reg(0x61, 0xD9) ||
      !write_reg(0x64, 0x8F) || !write_reg(0x10, 0xE4) ||
      !write_reg(0x00, 0x01) || !write_reg(0x03, 0xC3) ||
      !write_reg(0x24, 0x6A) || !write_reg(0x25, 0x0A)) {
    Serial.println("ES8389 board configuration failed");
    return false;
  }

  digitalWrite(pa_pin, HIGH);
  // Use the maximum ES8389 DAC output level for this loopback test.
  return set_output_volume(0xBF);
}

bool ES8389::set_output_volume(uint8_t volume) {
  bool result = write_reg(0x46, volume) && write_reg(0x47, volume);
  if (!result) {
    Serial.println("ES8389 output volume configuration failed");
  }
  return result;
}

bool ES8389::read_reg(uint8_t reg, uint8_t *value) {
  _i2c->beginTransmission(es8389_addr);
  if (_i2c->write(reg) != 1 || _i2c->endTransmission(false) != 0) {
    return false;
  }
  if (_i2c->requestFrom(es8389_addr, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  *value = _i2c->read();
  return true;
}

bool ES8389::write_reg(uint8_t reg, uint8_t value) {
  _i2c->beginTransmission(es8389_addr);
  if (_i2c->write(reg) != 1 || _i2c->write(value) != 1) {
    return false;
  }
  return _i2c->endTransmission() == 0;
}

bool ES8389::update_bits(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t reg_value;
  if (!read_reg(reg, &reg_value)) {
    return false;
  }
  reg_value = (reg_value & ~mask) | (value & mask);
  return write_reg(reg, reg_value);
}
