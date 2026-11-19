# User LED

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Turns on both user LEDs (GPIO2 and GPIO3, active low) at startup.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

Both user LEDs light up.

## Troubleshooting

None yet.
