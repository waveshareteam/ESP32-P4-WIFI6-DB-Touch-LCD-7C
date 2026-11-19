# GPIO Console

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Configures and tests GPIOs interactively through a serial console built on the Arduino-ESP32 `Console` library.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

1. Install the `esp32` board package (Arduino-ESP32) in the version listed in the repository README.
2. Install the board definition from [`examples/arduino/examples/esp32`](../esp32/README.md) and install any libraries the sketch includes.
3. Select the board and the **Tools** menu options that match the FQBN in `.github/product.json`:

   | Tools menu | Value |
   | --- | --- |
   | Board | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. Open `gpio.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Enter `help` in the serial monitor (line ending `Newline`) to list the commands `gpio read`, `gpio write`, `gpio mode` and `led`; the user LED follows `led on` and `led off`.

## Troubleshooting

The user LED on GPIO2 is active low. Do not drive GPIOs used by on-board peripherals while those peripherals are active.
