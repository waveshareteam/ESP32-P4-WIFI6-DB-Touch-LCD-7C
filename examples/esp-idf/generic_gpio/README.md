# GPIO Input/Output Monitor

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows the live level of the expansion-header GPIOs (`32, 33, 49, 50, 51, 48, 47, 46, 36, 25, 24, 5, 4`) as a scrollable LVGL grid on the display. A switch at the top changes all of them between input and output, and output cards toggle their level when touched.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

The initial GPIO direction is set under `Generic GPIO Example` in `idf.py menuconfig`; the default is input.

## Expected Result

The display shows one card per GPIO with its current level. Switch to output mode only when nothing external is connected to these pins.

## Troubleshooting

None yet.
