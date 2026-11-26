# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## 功能

打印芯片型号、版本、CPU、Flash 容量、PSRAM 容量和剩余堆内存。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

1. 安装示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `board_check.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器输出开发板信息。

## 常见问题

暂无。
