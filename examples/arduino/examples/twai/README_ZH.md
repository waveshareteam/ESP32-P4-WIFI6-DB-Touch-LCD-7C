# TWAI/CAN Receiver

[English](README.md) | [简体中文](README_ZH.md)

## 功能

以 500 kbit/s 在只听模式下接收 CAN 报文，打印每帧的 ID、DLC 和数据；接受所有 ID，不发送数据。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 连接到 TWAI/CAN 接口、速率为 500 kbit/s 的 CAN 总线

## 怎么运行

1. 安装示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `twai.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

总线上出现报文时，串口监视器打印对应内容。

## 常见问题

暂无。
