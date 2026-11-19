# MIPI-CSI Camera

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes an OV5647 MIPI-CSI camera and the MIPI-DSI LCD, and shows the live camera image on the LCD.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- OV5647 camera on the MIPI-CSI connector
- 7-inch MIPI-DSI display (on board)

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `mipi_csi.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The LCD shows the live camera image; the log prints `OV5647 direct display started` once the camera is detected.

## Troubleshooting

The LCD uses the local `lcd_ek79007.cpp` instead of `ESP32_Display_Panel` to avoid an old/new I2C driver conflict. The prebuilt `esp_video` library probes several sensors, so probe-failure logs for other sensors are expected.
