# I2C Scanner

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Scans the default I2C bus (SDA GPIO7, SCL GPIO8) and prints the 7-bit address of every device found.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- Optional: an external I2C device

## How to Run

1. Install sp32 by Espressif Systems **3.3.12** and any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `i2c.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor lists the detected addresses and rescans every 5 seconds.

## Troubleshooting

None yet.
