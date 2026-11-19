# UART RS232 Echo

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Echoes every byte received on the on-board RS232 port (TX GPIO28, RX GPIO29) back to the sender.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- An external RS232 device

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

Data sent from the RS232 device is returned unchanged; the serial monitor shows the UART configuration.

## Troubleshooting

None yet.
