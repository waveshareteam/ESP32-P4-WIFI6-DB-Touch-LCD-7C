# PCF85063A RTC

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads the on-board PCF85063A real-time clock over I2C (address `0x51`) and prints the date and time every second.

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
3. Open `rtc.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor prints the RTC time every second. If the RTC time is invalid, it is initialized with the firmware build time and kept across restarts.

## Troubleshooting

Set `rtc_set_time_on_boot` to `true` to overwrite the RTC with the build time once, then set it back to `false`.
