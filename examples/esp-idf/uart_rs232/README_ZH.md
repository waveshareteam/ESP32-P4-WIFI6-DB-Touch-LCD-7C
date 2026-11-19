# UART RS232 Echo

[English](README.md) | [简体中文](README_ZH.md)

## 功能

把板载 RS232 接口（TX GPIO28、RX GPIO29）收到的数据原样回传。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 外部 RS232 设备

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

RS232 设备发送的数据被原样返回，串口监视器显示 UART 配置。

## 常见问题

暂无。
