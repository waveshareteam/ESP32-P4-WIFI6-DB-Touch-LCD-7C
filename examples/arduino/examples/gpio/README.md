# GPIO Console

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Configures and tests GPIOs interactively through a serial console built on the Arduino-ESP32 `Console` library.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `gpio.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Enter `help` in the serial monitor (line ending `Newline`) to list the commands `gpio read`, `gpio write`, `gpio mode` and `led`; the user LED follows `led on` and `led off`.

## Troubleshooting

The user LED on GPIO2 is active low. Do not drive GPIOs used by on-board peripherals while those peripherals are active.
