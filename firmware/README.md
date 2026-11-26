# Firmware

[简体中文](README_ZH.md)

| File | Purpose | Flash target | Offset |
| --- | --- | --- | --- |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin) | ESP32-P4 factory demo firmware | ESP32-P4 | `0x0` |
| [ESP32-C5/ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin](ESP32-C5/ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin) | ESP-Hosted Wi-Fi and Bluetooth LE coprocessor firmware (backup only) | ESP32-C5 | — |

Select the ESP32-P4 download port and run from this directory:

```bash
python -m esptool --chip esp32p4 erase-flash
python -m esptool --chip esp32p4 write-flash 0x0 ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin
```

> **Note on ESP32-C5 firmware:**
> The binary under `ESP32-C5/` is kept for backup purposes only, and prebuilt C5 flashing is no longer provided or supported. If you need to build or update the ESP32-C5 coprocessor firmware, please build it yourself from the official repository: <https://github.com/espressif/esp-hosted-mcu>.
