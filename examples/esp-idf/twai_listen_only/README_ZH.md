# TWAI Listen-Only

[English](README.md) | [简体中文](README_ZH.md)

## 功能

以只听模式监视 TWAI/CAN 总线，并通过验收滤波器接收指定消息 ID 的帧。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 有数据的 CAN 总线，例如运行 `twai_sender` 示例的节点

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

串口监视器记录每个通过滤波的帧以及总线错误。

## 常见问题

暂无。
