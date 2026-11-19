# MIPI-DSI LCD

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the 7-inch EK79007 MIPI-DSI LCD (1024 × 600) with the `ESP32_Display_Panel` library and shows color bars.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- 7-inch MIPI-DSI display (on board)

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `mipi_dsi.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The LCD shows color bars.

## Troubleshooting

Install the `ESP32_Display_Panel` and `esp-lib-utils` libraries first. The configuration headers `esp_panel_drivers_conf.h` and `esp_utils_conf.h` are in the sketch folder.
