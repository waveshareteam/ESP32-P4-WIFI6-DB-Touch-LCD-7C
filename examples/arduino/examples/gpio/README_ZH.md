# GPIO Console

[English](README.md) | [简体中文](README_ZH.md)

## 功能

基于 Arduino-ESP32 的 `Console` 库，通过串口控制台交互式配置和测试 GPIO。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

1. 安装 sp32 by Espressif Systems **3.3.12** 开发板包以及示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `gpio.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

在串口监视器（行尾选择 `Newline`）输入 `help` 列出 `gpio read`、`gpio write`、`gpio mode` 和 `led` 命令；用户 LED 随 `led on`、`led off` 变化。

## 常见问题

GPIO2 上的用户 LED 为低电平点亮。不要在板载外设工作时操作其占用的 GPIO。
