# BLE 外设 GATT

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ESP-Hosted，借助 ESP32-C5-MINI-1 协处理器广播一个带 GATT 服务的 BLE 外设。示例运行在 ESP32-P4 上，ESP32-C5-MINI-1 运行 `firmware/` 中的 ESP-Hosted 协处理器固件。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-10.1 开发板
- 一部装有 BLE 调试工具的手机或电脑

## 怎么运行

1. 用 [`firmware`](../../../firmware/README_ZH.md) 目录中的 `*_C5-Hosted_*.bin` 固件给 ESP32-C5-MINI-1 协处理器烧录一次。
2. 编译本示例并烧录到 ESP32-P4：

   ```bash
   python -m pip install --upgrade esp-bmgr-assist
   idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
   idf.py set-target esp32p4 build flash monitor
   ```

## 预期现象

开发板以 `esp-hosted-nim-gatt` 名称广播；连接、写特征值和订阅通知都会在 ESP32-P4 日志中显示。

## 常见问题

- 烧录本示例不会更新协处理器，`*_C5-Hosted_*.bin` 固件需要单独烧录。
