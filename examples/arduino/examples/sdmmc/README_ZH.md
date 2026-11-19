# microSD (SDMMC)

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 4 位 SDMMC 初始化板载 microSD 卡槽，打印卡片和文件系统容量，并读写测试文件。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- microSD 卡

## 怎么运行

1. 安装仓库 README 中列出版本的 `esp32` 开发板包（Arduino-ESP32）。
2. 按[板卡文件说明](../esp32/README_ZH.md)安装本地板卡定义，并安装示例所包含的库。
3. 按 `.github/product.json` 的 FQBN 选择开发板和 **工具** 菜单选项：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. 打开 `sdmmc.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器显示卡片信息以及 `/arduino_test.txt` 的写入和读取结果。

## 常见问题

暂无。
