# 固件

[English](README.md)

| 文件 | 用途 | 烧录目标 | 偏移地址 |
| --- | --- | --- | --- |
| [ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin](ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin) | ESP32-P4 出厂演示固件 | ESP32-P4 | `0x0` |
| [ESP32-C5/ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin](ESP32-C5/ESP32-P4-WIFI6-DB-Touch-LCD-7C_ESP32-C5-ESP-Hosted_20261118.bin) | ESP-Hosted Wi-Fi 和低功耗蓝牙协处理器固件（仅作备份） | ESP32-C5 | — |

选择 ESP32-P4 的下载端口，从本目录执行：

```bash
python -m esptool --chip esp32p4 erase-flash
python -m esptool --chip esp32p4 write-flash 0x0 ESP32-P4-WIFI6-DB-Touch-LCD-7C_FactoryOnly_20261126.bin
```

> **关于 ESP32-C5 固件的说明：**
> `ESP32-C5/` 目录下的固件仅作为备份保存，不再提供 C5 固件烧录支持。如需更新或自定义 ESP32-C5 协处理器固件，请前往官方仓库 <https://github.com/espressif/esp-hosted-mcu> 自行构建。
