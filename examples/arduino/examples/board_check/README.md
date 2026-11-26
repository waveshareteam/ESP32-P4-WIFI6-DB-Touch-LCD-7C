# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Prints the chip model, revision, CPU, Flash size, PSRAM size and free heap.

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
3. Open `board_check.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor shows the board information.

## Troubleshooting

None yet.
