# RS232 Bridge

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 USB 串口和板载 RS232 收发器之间双向桥接。RS232 端为 9600 8N1，TX GPIO28、RX GPIO29。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 外部 RS232 设备

## 怎么运行

1. 安装 sp32 by Espressif Systems **3.3.12** 开发板包以及示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `rs232.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

在串口监视器（115200 波特率）输入的内容发送到 RS232 设备，设备返回的数据显示在监视器中。

## 常见问题

暂无。
