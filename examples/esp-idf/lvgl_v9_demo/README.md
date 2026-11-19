# LVGL v9 Benchmark

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs the LVGL v9 benchmark demo on the display configured for the selected board through `esp_board_manager` and `esp_lvgl_adapter`.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

The board display shows the LVGL v9 benchmark scenes and the serial monitor logs `LVGL benchmark started`.

## Troubleshooting

None yet.
