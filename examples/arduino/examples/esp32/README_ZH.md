# Arduino-ESP32 本地板级文件说明

[English](README.md) | [简体中文](README_ZH.md)

## 来源与版本

本目录中的文件来自本机 Arduino-ESP32 `3.3.11`：

```text
Arduino\hardware\espressif\esp32
```

当前保留的是本项目使用的板级文件：

- `boards.txt`：注册 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C` 板卡。
- `variants/waveshare_esp32_p4_wifi6_db_touch_lcd_7c/pins_arduino.h`：定义 7C 板卡的 GPIO、MIPI-DSI LCD、摄像头 SCCB、触摸、音频、SDMMC 和 ESP-Hosted 引脚。

本项目使用的版本基线为 Arduino-ESP32 `3.3.11`