#pragma once

#include <Arduino.h>
#include <Wire.h>

class ES8389 {
public:
  bool begin(TwoWire &i2c, uint8_t pa_pin);
  bool set_output_volume(uint8_t volume);

private:
  bool write_reg(uint8_t reg, uint8_t value);
  bool read_reg(uint8_t reg, uint8_t *value);
  bool update_bits(uint8_t reg, uint8_t mask, uint8_t value);

  TwoWire *_i2c = nullptr;
};
