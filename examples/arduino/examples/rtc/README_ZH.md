# PCF85063A RTC

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 I2C（地址 `0x51`）读取板载 PCF85063A 实时时钟，每秒打印日期和时间。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

1. 安装仓库 README 中列出版本的 `esp32` 开发板包（Arduino-ESP32）。
2. 按[板卡文件说明](../esp32/README_ZH.md)安装本地板卡定义，并安装示例所包含的库。
3. 按 `.github/product.json` 的 FQBN 选择开发板和 **工具** 菜单选项：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. 打开 `rtc.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器每秒打印 RTC 时间。RTC 时间无效时用固件编译时间初始化，重启后保持。

## 常见问题

将 `rtc_set_time_on_boot` 改为 `true` 可用编译时间覆盖一次 RTC，之后请改回 `false`。
