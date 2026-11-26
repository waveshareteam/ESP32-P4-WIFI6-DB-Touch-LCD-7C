# TWAI/CAN Receiver

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Receives CAN frames in listen-only mode at 500 kbit/s and prints each frame's ID, DLC and data. It accepts all IDs and never transmits.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- A running CAN bus at 500 kbit/s connected to the TWAI/CAN port

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `twai.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The serial monitor prints each frame seen on the bus.

## Troubleshooting

None yet.
