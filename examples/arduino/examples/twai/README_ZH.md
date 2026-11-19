# TWAI/CAN Receiver

[English](README.md) | [简体中文](README_ZH.md)

## 功能

以 500 kbit/s 在只听模式下接收 CAN 报文，打印每帧的 ID、DLC 和数据；接受所有 ID，不发送数据。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 连接到 TWAI/CAN 接口、速率为 500 kbit/s 的 CAN 总线

## 怎么运行

1. 安装仓库 README 中列出版本的 `esp32` 开发板包（Arduino-ESP32）。
2. 按[板卡文件说明](../esp32/README_ZH.md)安装本地板卡定义，并安装示例所包含的库。
3. 按 `.github/product.json` 的 FQBN 选择开发板和 **工具** 菜单选项：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. 打开 `twai.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

总线上出现报文时，串口监视器打印对应内容。

## 常见问题

暂无。
