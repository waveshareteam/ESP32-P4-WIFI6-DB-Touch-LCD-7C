# UART RS485 Echo

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用 UART 驱动的 RS485 半双工模式，把板载 RS485 接口收到的数据回传给发送方。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 外部 RS485 设备或 USB 转 RS485 适配器

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

RS485 端发送的数据被回传，串口监视器记录收到的数据。

## 常见问题

暂无。
