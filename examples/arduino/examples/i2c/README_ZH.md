# I2C Scanner

[English](README.md) | [简体中文](README_ZH.md)

## 功能

扫描默认 I2C 总线（SDA GPIO7、SCL GPIO8），打印检测到的每个器件的 7 位地址。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 可选：外接 I2C 器件

## 怎么运行

1. 安装仓库 README 中列出版本的 `esp32` 开发板包（Arduino-ESP32）。
2. 按[板卡文件说明](../esp32/README_ZH.md)安装本地板卡定义，并安装示例所包含的库。
3. 按 `.github/product.json` 的 FQBN 选择开发板和 **工具** 菜单选项：

   | 工具菜单 | 值 |
   | --- | --- |
   | 开发板 | Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C |
   | PSRAM | Enabled |

4. 打开 `i2c.ino` 并上传。打开串口监视器，波特率 115200。

## 预期现象

串口监视器列出检测到的地址，每 5 秒重新扫描一次。

## 常见问题

暂无。
