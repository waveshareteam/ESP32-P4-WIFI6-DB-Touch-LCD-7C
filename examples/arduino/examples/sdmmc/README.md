# microSD (SDMMC)

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the on-board microSD slot over 4-bit SDMMC, prints the card and file system capacity, and reads and writes a test file.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- microSD card

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `sdmmc.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor shows the card information and the write and read results of `/arduino_test.txt`.

## Troubleshooting

None yet.
