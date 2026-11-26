# BOOT 按键

[English](README.md) | [简体中文](README_ZH.md)

## 功能

读取板载 BOOT 按键（`Key2`，`BOOT_PIN` = GPIO35，低电平有效），带软件消抖，串口输出按下、短按、长按（≥ 1000 ms）和释放事件。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

1. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
2. 打开 `button.ino` 并上传，然后以 115200 波特率打开串口监视器。

## 预期现象

按下或长按板载 `Key2`（BOOT）按键时，串口监视器打印对应的短按、长按及释放事件。

## 常见问题

暂无。
