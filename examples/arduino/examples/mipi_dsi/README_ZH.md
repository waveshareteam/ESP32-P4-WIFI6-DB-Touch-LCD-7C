# MIPI-DSI LCD

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用 `ESP32_Display_Panel` 库初始化 7 英寸 EK79007 MIPI-DSI LCD（1024 × 600）并显示色条。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 7 英寸 MIPI-DSI 显示屏（板载）

## 怎么运行

1. 安装 `esp32 by Espressif Systems` **3.3.12** 开发板包，以及 `ESP32_Display_Panel`（`1.0.4`）和 `esp-lib-utils`（`0.2.3`）库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `mipi_dsi.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

LCD 显示彩色色条。

## 常见问题

需要先安装 `ESP32_Display_Panel`（`1.0.4`）和 `esp-lib-utils`（`0.2.3`）库。配置头文件 `esp_panel_drivers_conf.h` 和 `esp_utils_conf.h` 位于示例目录。
