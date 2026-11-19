# TWAI Sender

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 TWAI/CAN 总线上周期发送心跳帧和大数据包（默认 TX GPIO22、RX GPIO21）。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板
- 至少一个其他 CAN 节点，总线两端接 120 Ω 终端电阻

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

串口监视器记录已发送的帧和总线状态，总线上的监听节点能收到这些帧。

## 常见问题

没有其他节点时，可在 `menuconfig` 中启用自测模式。
