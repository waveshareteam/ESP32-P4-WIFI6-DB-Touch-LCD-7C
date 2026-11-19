# RS232 Bridge

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 USB 串口和板载 RS232 收发器之间双向桥接。RS232 端为 9600 8N1，TX GPIO28、RX GPIO29。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 外部 RS232 设备

## 怎么运行

1. 安装仓库 README 中列出版本的 `esp32` 开发板包（Arduino-ESP32）。
2. 按[板卡文件说明](../../esp32/README_ZH.md)安装本地板卡定义，并安装示例所包含的库。
3. 按 `.github/product.json` 的 FQBN 选择开发板和 **工具** 菜单选项：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. 打开 `rs232.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

在串口监视器（115200 波特率）输入的内容发送到 RS232 设备，设备返回的数据显示在监视器中。

## 常见问题

暂无。
