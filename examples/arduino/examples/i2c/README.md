# I2C Scanner

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Scans the default I2C bus (SDA GPIO7, SCL GPIO8) and prints the 7-bit address of every device found.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- Optional: an external I2C device

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `i2c.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor lists the detected addresses and rescans every 5 seconds.

## Troubleshooting

None yet.
