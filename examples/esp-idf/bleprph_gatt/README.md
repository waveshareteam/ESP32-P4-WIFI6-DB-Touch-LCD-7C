# BLE Peripheral GATT

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Advertises a BLE peripheral with a GATT service through the ESP32-C5-MINI-1 coprocessor using ESP-Hosted. The example runs on the ESP32-P4; the ESP32-C5-MINI-1 runs the ESP-Hosted coprocessor firmware from `firmware/`.

## Hardware Required

- ESP32-P4-WIFI6-DB-Touch-LCD-10.1 development board
- A phone or PC with a BLE scanner app

## How to Run

1. Flash the ESP32-C5-MINI-1 coprocessor once with the `*_C5-Hosted_*.bin` firmware from the [`firmware`](../../../firmware/README.md) folder.
2. Build and flash this example to the ESP32-P4:

   ```bash
   python -m pip install --upgrade esp-bmgr-assist
   idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
   idf.py set-target esp32p4 build flash monitor
   ```

## Expected Result

The board advertises as `esp-hosted-nim-gatt`; connecting, writing the characteristic and subscribing to notifications are logged on the ESP32-P4.

## Troubleshooting

- Flashing this example does not update the coprocessor; flash the `*_C5-Hosted_*.bin` firmware separately.
