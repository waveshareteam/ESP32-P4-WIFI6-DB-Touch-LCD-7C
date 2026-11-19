# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Prints the chip, Flash, PSRAM and heap information over the serial monitor and keeps running, so you can confirm that the toolchain, flashing and serial monitor work.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4 build flash monitor
```

## Expected Result

The serial monitor shows the board information and then periodic logs.

## Troubleshooting

None yet.
