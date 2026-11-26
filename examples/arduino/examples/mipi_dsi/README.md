# MIPI-DSI LCD

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the 7-inch EK79007 MIPI-DSI LCD (1024 × 600) with the `ESP32_Display_Panel` library and shows color bars.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- 7-inch MIPI-DSI display (on board)

## How to Run

1. Install `esp32 by Espressif Systems` **3.3.12** and the `ESP32_Display_Panel` (`1.0.4`) and `esp-lib-utils` (`0.2.3`) libraries.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `mipi_dsi.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The LCD shows color bars.

## Troubleshooting

Install the `ESP32_Display_Panel` (`1.0.4`) and `esp-lib-utils` (`0.2.3`) libraries first. The configuration headers `esp_panel_drivers_conf.h` and `esp_utils_conf.h` are in the sketch folder.
