# Wi-Fi STA

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 SDIO 上的 ESP-Hosted，借助 ESP32-C5-MINI-1 协处理器连接 Wi-Fi 热点。示例运行在 ESP32-P4 上，ESP32-C5-MINI-1 运行 `firmware/` 中的 ESP-Hosted 协处理器固件。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-10.1 开发板
- 一个 2.4 GHz 或 5 GHz Wi-Fi 热点

## 怎么运行

1. 用 [`firmware`](../../../firmware/README_ZH.md) 目录中的 `*_C5-Hosted_*.bin` 固件给 ESP32-C5-MINI-1 协处理器烧录一次。
2. 编译本示例并烧录到 ESP32-P4：

   ```bash
   python -m pip install --upgrade esp-bmgr-assist
   idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
   idf.py set-target esp32p4 build flash monitor
   ```

## 预期现象

ESP32-P4 日志显示 `got ip:` 和热点分配的 IP 地址。

## 常见问题

- 在 menuconfig 的 **Example Configuration** 中设置 SSID 和密码，默认值只是占位符。
- 烧录本示例不会更新协处理器，`*_C5-Hosted_*.bin` 固件需要单独烧录。
