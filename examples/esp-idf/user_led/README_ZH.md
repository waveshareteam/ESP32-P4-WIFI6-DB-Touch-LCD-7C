# User LED

[English](README.md) | [简体中文](README_ZH.md)

## 功能

启动后点亮两个用户 LED（GPIO2 和 GPIO3，低电平点亮）。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

两个用户 LED 点亮。

## 常见问题

暂无。
