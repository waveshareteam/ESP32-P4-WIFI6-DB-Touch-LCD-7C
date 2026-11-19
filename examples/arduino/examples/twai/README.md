# TWAI/CAN Receiver

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Receives CAN frames in listen-only mode at 500 kbit/s and prints each frame's ID, DLC and data. It accepts all IDs and never transmits.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- A running CAN bus at 500 kbit/s connected to the TWAI/CAN port

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `twai.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor prints each frame seen on the bus.

## Troubleshooting

None yet.
