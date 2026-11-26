# BOOT Button

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads the on-board BOOT button (`Key2`, `BOOT_PIN` = GPIO35, active LOW) with software debouncing and prints press, short-press, long-press (>= 1000 ms), and release events over Serial.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

1. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
2. Open `button.ino`, upload it and open the serial monitor at 115200 baud.

## Expected Result

Pressing or holding the on-board `Key2` (BOOT) button prints short-press, long-press, and release events in the serial monitor.

## Troubleshooting

None.
