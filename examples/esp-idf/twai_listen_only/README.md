# TWAI Listen-Only

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Monitors the TWAI/CAN bus in listen-only mode and receives frames of one message ID through an acceptance filter.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- A CAN bus carrying traffic, for example from the `twai_sender` example

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

The serial monitor logs each filtered frame and bus errors.

## Troubleshooting

None yet.
