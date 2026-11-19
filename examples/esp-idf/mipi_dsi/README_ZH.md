# MIPI-DSI 显示色条

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 `esp_board_manager` 初始化 1024 x 600 的 EK79007 MIPI-DSI 屏幕和背光，再由 DPI 面板驱动生成垂直彩条图案。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

背光点亮，屏幕显示垂直彩条，持续两秒。

## 常见问题

- 屏幕不亮时，检查 DSI 排线方向和屏幕供电。
