# RS232 Bridge

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Bridges the USB serial port and the on-board RS232 transceiver in both directions. The RS232 side uses 9600 8N1 on TX GPIO28 and RX GPIO29.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- An external RS232 device

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `rs232.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Text typed in the serial monitor (115200 baud) is sent to the RS232 device, and data from the device appears in the monitor.

## Troubleshooting

None yet.
