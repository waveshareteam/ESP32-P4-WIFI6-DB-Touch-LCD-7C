# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## 功能

打印芯片型号、版本、CPU、Flash 容量、PSRAM 容量和剩余堆内存。

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

4. 打开 `board_check.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器输出开发板信息。

## 常见问题

暂无。
