# ES8389 I2S Loopback

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the on-board ES8389 codec and forwards the two differential microphone channels to the speaker output.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- Speaker connected to the on-board speaker output

## How to Run

1. Install any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `i2s.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

Sound picked up by the microphones is played through the speaker; the serial monitor shows the initialization status.

## Troubleshooting

Keep the volume low on the first run to avoid acoustic feedback.
