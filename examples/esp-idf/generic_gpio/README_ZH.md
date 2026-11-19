# GPIO 输入输出监控

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在屏幕上以可滚动的 LVGL 网格显示扩展排针 GPIO（`32, 33, 49, 50, 51, 48, 47, 46, 36, 25, 24, 5, 4`）的实时电平。顶部开关可在输入和输出模式之间切换，输出模式下点击卡片可翻转电平。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

初始 GPIO 方向在 `idf.py menuconfig` 的 `Generic GPIO Example` 中设置，默认为输入。

## 预期现象

屏幕上每个 GPIO 一张卡片，显示当前电平。仅在这些引脚没有外接设备时才切换到输出模式。

## 常见问题

暂无
