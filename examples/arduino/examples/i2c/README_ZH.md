# I2C Scanner

[English](README.md) | [简体中文](README_ZH.md)

## 功能

扫描默认 I2C 总线（SDA GPIO7、SCL GPIO8），打印检测到的每个器件的 7 位地址。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 可选：外接 I2C 器件

## 怎么运行

1. 安装 sp32 by Espressif Systems **3.3.12** 开发板包以及示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `i2c.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器列出检测到的地址，每 5 秒重新扫描一次。

## 常见问题

暂无。
