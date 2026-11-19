# Firmware

[简体中文](README_ZH.md)

| File | Purpose | Flash target | Offset |
| --- | --- | --- | --- |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin) | ESP32-P4 factory demo firmware | ESP32-P4 | `0x0` |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin) | ESP-Hosted Wi-Fi and Bluetooth LE coprocessor firmware | ESP32-C5 | `0x0` |

Select the ESP32-C5's download port and write the complete BIN at `0x0`.

Select the ESP32-P4 download port and run from this directory:

```bash
python -m esptool --chip esp32p4 erase-flash
python -m esptool --chip esp32p4 write-flash 0x0 ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin
```
