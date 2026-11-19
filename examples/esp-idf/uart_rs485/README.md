# UART RS485 Echo

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Echoes data received on the on-board RS485 port back to the sender, using the UART driver in RS485 half-duplex mode.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- An external RS485 device or USB-to-RS485 adapter

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

Data sent from the RS485 side is echoed back; the serial monitor logs the received data.

## Troubleshooting

None yet.
