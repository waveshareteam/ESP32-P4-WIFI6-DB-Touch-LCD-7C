# MIPI-CSI Camera

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes an OV5647 MIPI-CSI camera and the MIPI-DSI LCD, and shows the live camera image on the LCD.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- OV5647 camera on the MIPI-CSI connector
- 7-inch MIPI-DSI display (on board)

## How to Run

1. Install `esp32 by Espressif Systems` **3.3.12** and any libraries the sketch includes.
2. Select the board and serial port in **Tools**:

   | Tools menu | Value |
   | --- | --- |
   | Board | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. Open `mipi_csi.ino` and upload. Open the serial monitor at 115200 baud.

## Expected Result

The LCD shows the live camera image; the log prints `OV5647 direct display started` once the camera is detected.

## Troubleshooting

- **`dw-gdma: dw_gdma_channel_register_event_callbacks(675): user context not in internal RAM` / `Camera capture start failed`**: Make sure you are using Arduino-ESP32 **3.3.12** with `ESP32P4 Dev Module`. Prebuilt core versions such as 4.0.0 where `CONFIG_DW_GDMA_ISR_IRAM_SAFE=y` and `CONFIG_CAM_CTLR_MIPI_CSI_ISR_CACHE_SAFE=n` allocate the CSI controller context in PSRAM, which DW-GDMA rejects; switch to Arduino-ESP32 **3.3.12** (or use the [ESP-IDF `mipi_csi` example](../../../esp-idf/mipi_csi/README.md)).
- The LCD uses the local `lcd_ek79007.cpp` instead of `ESP32_Display_Panel` to avoid an old/new I2C driver conflict. The prebuilt `esp_video` library probes several sensors, so probe-failure logs for other sensors are expected.
