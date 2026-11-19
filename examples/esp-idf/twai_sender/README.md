# TWAI Sender

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Sends periodic heartbeat frames and large data packets on the TWAI/CAN bus (TX GPIO22, RX GPIO21 by default).

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-7C development board
- At least one other CAN node, with 120 Ω termination at both bus ends

## How to Run

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## Expected Result

The serial monitor logs the transmitted frames and bus status; a listener on the bus receives them.

## Troubleshooting

Enable self-test mode in `menuconfig` to run without another node.
