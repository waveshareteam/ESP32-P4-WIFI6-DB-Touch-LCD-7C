# LVGL v9 基准测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 `esp_board_manager` 按所选板卡的配置初始化显示屏、触摸和背光，再由 `esp_lvgl_adapter` 运行 LVGL v9 benchmark。

## 硬件需求

- ESP32-P4-WIFI6-DB-Touch-LCD-7C 开发板

## 怎么运行

```bash
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build flash monitor
```

## 预期现象

板载屏幕显示 LVGL v9 benchmark 测试画面，串口输出 `LVGL benchmark started`。

## 常见问题

暂无。
