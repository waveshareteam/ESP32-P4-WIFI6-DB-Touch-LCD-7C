# RS485 Bridge

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Bridges the USB serial port and the on-board RS485 transceiver in half duplex. The RS485 side uses 9600 8N1 on TX GPIO26, RX GPIO27 and direction control GPIO34.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- An external RS485 device

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `rs485.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Text typed in the serial monitor (115200 baud) is sent to the RS485 bus, and received data appears in the monitor.

## Troubleshooting

None yet.
