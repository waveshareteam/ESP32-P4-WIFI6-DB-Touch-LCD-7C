# 开发板检查

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过串口监视器打印芯片、Flash、PSRAM 和堆信息并持续运行，用于确认工具链、烧录流程和串口监视器工作正常。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

串口监视器输出开发板信息，之后周期性打印日志。

## 常见问题

暂无。
