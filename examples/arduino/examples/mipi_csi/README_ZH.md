# MIPI-CSI Camera

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化 OV5647 MIPI-CSI 摄像头和 MIPI-DSI LCD，并在 LCD 上显示摄像头实时画面。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 接在 MIPI-CSI 接口的 OV5647 摄像头
- 7 英寸 MIPI-DSI 显示屏（板载）

## 怎么运行

1. 安装示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `mipi_csi.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

LCD 显示摄像头实时画面；检测到摄像头后日志输出 `OV5647 direct display started`。

## 常见问题

LCD 使用同目录的 `lcd_ek79007.cpp` 而不是 `ESP32_Display_Panel`，以避免新旧 I2C 驱动冲突。预编译的 `esp_video` 库会依次探测多种传感器，出现其他传感器的探测失败日志属于正常现象。
