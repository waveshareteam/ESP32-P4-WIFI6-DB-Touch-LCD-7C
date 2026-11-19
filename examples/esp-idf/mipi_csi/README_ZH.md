# MIPI-CSI 摄像头预览

[English Version](./README.md)

## 功能

示例通过 `esp_board_manager` 按名称初始化 `display_lcd`、`lcd_brightness` 和 `camera`。通过 `esp_video_capture_service` 获取 RGB565 帧，交给 `esp_video_render` 显示。默认使用直接 LCD backend。开启 `CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT` 可切换到 LVGL backend，由 `esp_lvgl_port` 管理 DSI RGB565 双缓冲 direct mode。应用不再自行管理 PPA、帧复制、LCD 刷新回调和帧缓冲切换，这些工作交由组件处理。

## 硬件需求

- ESP32-P4-WIFI6-DB Touch LCD 7C，1024 × 600 EK79007 MIPI-DSI 屏幕。
- 受 `esp_video` 支持的 MIPI-CSI 摄像头，板级默认 OV5647 RAW8 800 × 800、50 FPS。
- LCD、背光、摄像头 SCCB/I2C 和共享 MIPI LDO 参数来自 Board Manager 板级配置。
- 构建前运行 `idf.py bmgr` 为 `esp32_p4_wifi6_db_touch_lcd_7c` 生成 `components/gen_bmgr_codes`。修改板级 YAML 后需重新运行。
- 应用支持未旋转、未镜像的 DSI RGB565_LE 面板；尺寸从 LCD 配置读取，并检查像素格式。LCD 初始化前，应用将帧缓冲数量运行时覆盖为 2，供 LVGL direct mode 使用，同时兼容 `esp_video_render` 1.1 直接 LCD backend 的限制，不修改板级 YAML。
- 示例不初始化触摸、音频或文件系统。

## 怎么运行

需要 ESP-IDF v5.5 或更高版本。由用户在终端执行：

```powershell
cd examples/esp-idf/mipi_csi
idf.py set-target esp32p4
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build
idf.py -p PORT flash monitor
```

摄像头型号及输出格式可在 `idf.py menuconfig` 的 `Espressif Camera Sensors` 菜单中选择。默认格式以生成的板级配置为准。

## 预期现象

LCD 背光亮起并显示实时画面。capture service 管理三个摄像头缓冲，为持帧渲染期间的采集留出余量，每帧交给 `esp_video_render_stream_write()` 后释放。单路 render stream 同步渲染并关闭额外帧缓存；`stream_write()` 返回后再释放 capture service 的帧，不启用异步渲染。

`gmf_video` 提供 `vid_ppa` 硬件缩放元素。完整画面按原始宽高比缩放到屏幕范围内，居中显示，空白区域为黑色。默认 800 × 800 摄像头配合 1024 × 600 屏幕时，预览区域为 600 × 600。应用不额外镜像画面。每两秒输出实际提交帧率、平均取帧等待时间和平均渲染耗时日志，配置帧率不代表实测显示帧率。

`sdkconfig.defaults` 默认关闭 LVGL backend；text widget 关闭，保留 widget 基础支持。可选 LVGL 路径预设 RGB565 和 10 ms 刷新周期，启用后创建用于显示视频的 image 对象，不额外添加控制界面。`esp_video_render` 1.1.0 的 manifest 通过 text widget 选项判断 FreeType 依赖；关闭其父级 widget 选项可能使该符号缺失于 `sdkconfig.json`，导致 `Missing required kconfig option after retry`。`esp_video_render` 在 ESP32-P4 上仍可能在组件内部使用 PPA 合成；移除应用层 PPA 代码不等于禁用组件内部的硬件加速。

## 常见问题

- 先运行 [04_mipi_dsi](../mipi_dsi/) 检查显示路径。
- 摄像头初始化或打开失败时，检查 FPC、供电、传感器选择及板级 I2C 配置。
- 确认 PSRAM 已启用；缓冲分配失败会立即报告错误。
- `ESP_ERR_NOT_SUPPORTED` 时检查 LCD 类型、RGB565 格式和方向；LCD 初始化还要求双缓冲运行时覆盖成功。

LVGL backend 的 `Preview` FPS 统计仍表示应用提交帧率，不能直接视为 LCD 实际刷新帧率；对比时也需观察实际画面流畅度。
