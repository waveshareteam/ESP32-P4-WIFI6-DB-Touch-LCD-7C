# ES8389 I2S Loopback

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the on-board ES8389 codec and forwards the two differential microphone channels to the speaker output.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- Speaker connected to the on-board speaker output

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `i2s.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Sound picked up by the microphones is played through the speaker; the serial monitor shows the initialization status.

## Troubleshooting

Keep the volume low on the first run to avoid acoustic feedback.
