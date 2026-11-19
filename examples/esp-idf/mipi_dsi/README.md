# MIPI-DSI Display Color Bars

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initializes the 1024 x 600 EK79007 MIPI-DSI display and its backlight through `esp_board_manager`, then asks the DPI panel driver to draw a vertical color-bar pattern.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

The backlight turns on and the display shows vertical color bars for two seconds.

## Troubleshooting

- If the display stays blank, check the DSI cable orientation and the panel power.
