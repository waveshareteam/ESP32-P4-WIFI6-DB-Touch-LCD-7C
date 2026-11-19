# 更新记录

[English](CHANGELOG.md) | [简体中文](CHANGELOG_ZH.md)

## v0.3.1 2026-08-05

- 根据 Board Manager 的显示配置，将 JPEG 帧解码为 RGB565 或 RGB888 帧缓冲格式。

## v0.3.0 2026-07-29

- 将显示、触摸、背光和音频初始化迁移至 ESP Board Manager。
- 使用统一的 Board Manager 设备名称，使兼容的 ESP32-P4、ESP32-S31 和 ESP32-S3 板卡共享应用代码。
- 移除示例中的本地 BSP 副本。
- 将硬件 JPEG 的 MCU 填充与 Board Manager 的显示分辨率分开处理。
- 根据 Board Manager 的显示配置选择 RGB565 字节序。

## v0.2.0 2026-05-25

- 支持 ESP32-S31-Korvo（800 × 480，60 FPS）。
- 重构 esp32_p4_function_ev_board BSP。

## v0.1.0

- 初始版本。
