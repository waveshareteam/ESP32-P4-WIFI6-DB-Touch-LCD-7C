# microSD (SDMMC)

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the on-board microSD slot over 4-bit SDMMC, prints the card and file system capacity, and reads and writes a test file.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- microSD card

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `sdmmc.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor shows the card information and the write and read results of `/arduino_test.txt`.

## Troubleshooting

None yet.
