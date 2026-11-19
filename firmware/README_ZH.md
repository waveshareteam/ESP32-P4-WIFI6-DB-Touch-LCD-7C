# 固件

[English](README.md)

| 文件 | 用途 | 烧录目标 | 偏移地址 |
| --- | --- | --- | --- |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin) | ESP32-P4 出厂演示固件 | ESP32-P4 | `0x0` |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin) | ESP-Hosted Wi-Fi 和低功耗蓝牙协处理器固件 | ESP32-C5 | `0x0` |

选择 ESP32-C5 的下载端口，将完整 BIN 写入 `0x0`。

选择 ESP32-P4 的下载端口，从本目录执行：

```bash
python -m esptool --chip esp32p4 erase-flash
python -m esptool --chip esp32p4 write-flash 0x0 ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261119.bin
```
