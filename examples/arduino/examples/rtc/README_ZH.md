# PCF85063A RTC

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 I2C（地址 `0x51`）读取板载 PCF85063A 实时时钟，每秒打印日期和时间。

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
3. 打开 `rtc.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器每秒打印 RTC 时间。RTC 时间无效时用固件编译时间初始化，重启后保持。

## 常见问题

将 `rtc_set_time_on_boot` 改为 `true` 可用编译时间覆盖一次 RTC，之后请改回 `false`。
