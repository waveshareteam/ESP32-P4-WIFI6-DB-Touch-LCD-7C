# RS232 Bridge

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Bridges the USB serial port and the on-board RS232 transceiver in both directions. The RS232 side uses 9600 8N1 on TX GPIO28 and RX GPIO29.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- An external RS232 device

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `rs232.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Text typed in the serial monitor (115200 baud) is sent to the RS232 device, and data from the device appears in the monitor.

## Troubleshooting

None yet.
