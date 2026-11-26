# ES8389 I2S Loopback

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化板载 ES8389 编解码器，并将两路差分麦克风信号转发到扬声器输出。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 连接到板载扬声器输出的扬声器

## 怎么运行

1. 安装示例所包含的库。
2. 在「工具」菜单中按以下配置选择开发板和串口：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | ESP32P4 Dev Module |
   | PSRAM | Enabled |
   | Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
3. 打开 `i2s.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

麦克风拾取的声音通过扬声器播放，串口监视器显示初始化状态。

## 常见问题

首次运行请把音量调低，避免啸叫。
