# mipi_csi

## 作用

初始化 MIPI-CSI 摄像头和 MIPI-DSI LCD，将摄像头采集到的实时图像显示在 LCD 上。当前示例使用 OV5647 摄像头和 1024×600 EK79007 LCD 配置。

## 使用方法

1. 将 OV5647 摄像头连接到开发板的 MIPI-CSI 接口。
2. 将 LCD 连接到开发板的 MIPI-DSI 接口。
3. 使用 Arduino-ESP32 `3.3.11`，并在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C`。
4. 本示例不依赖 `ESP32_Display_Panel`。LCD 驱动由同目录的 `lcd_ek79007.cpp` 维护，使用 Arduino-ESP32 自带的原生 `esp_lcd` MIPI-DSI API。
   版本和本地板级文件说明见 [Arduino-ESP32 本地板级文件说明](../esp32/README.md)。
5. 打开 `mipi_csi.ino`，编译并上传。
6. 打开串口监视器，波特率设置为 `115200`。
7. 初始化成功后，LCD 会显示摄像头实时画面。

## LCD

本示例使用 Arduino-ESP32 `3.3.11`（ESP-IDF `v5.5.5`）。`ESP32_Display_Panel 1.0.4` 不是本示例的依赖；如果它被编译进同一程序，就会引入旧版 I2C 驱动。Arduino-ESP32 内置的底层 `esp_video` 版本为 `2.3.0`。

在这个环境中，`ESP32_Display_Panel 1.0.4` 使用旧版 I2C 驱动，而 `ESP_Video 3.3.11` 的摄像头/SCCB 使用新版 `i2c_master` 驱动。两套驱动同时存在时，会触发新旧 I2C 驱动冲突：

```text
CONFLICT! driver_ng is not allowed to be used with this old driver
```

这个冲突不要求 LCD 本身实际使用 I2C 才会发生。当前版本的 `ESP32_Display_Panel` 会编译并链接使用旧版 `driver/i2c.h` 的 `esp_panel_host_i2c.cpp`，因此即使示例没有调用 LCD 的 I2C 接口，旧驱动的初始化路径仍可能在启动阶段被执行。

为避开这个冲突，本示例不引入 `ESP32_Display_Panel`，而是在同目录维护 `lcd_ek79007.cpp`，直接使用原生 `esp_lcd` MIPI-DSI API，并通过 GPIO 控制 LCD 复位和背光。这样既保留 `ESP_Video` 的新 I2C 摄像头驱动，也不要求使用者修改 Arduino 全局安装的官方库。

这是针对当前版本冲突的项目内兼容方案。未来只有在官方发布兼容新旧 I2C 驱动的版本后，才再评估是否恢复使用 `ESP32_Display_Panel`

## 摄像头

本示例使用 Arduino-ESP32 `3.3.11` 内置的 `ESP_Video`。该版本的 ESP32-P4 预编译 `esp_video` 库启用了多个 MIPI 摄像头的自动探测。初始化 OV5647 时会先尝试其他传感器，因此可能出现 `imx500`、`os04c10`、`ov2710` 等驱动的探测失败日志。这不是 OV5647 的硬件要求，而是当前库的构建配置。

日志出现 `PID=0x5647`，并随后输出 `OV5647 direct display started`，表示 OV5647 已识别并启动。

## LCD 配置

当前本地 EK79007 驱动固定使用以下 7C 板参数：

- 1024×600、RGB565
- MIPI-DSI 双通道，单通道 1000 Mbps
- DPI 时钟 52 MHz
- DSI PHY 使用 LDO3，电压 2500 mV
- LCD 复位 GPIO31，背光 GPIO30，背光低电平点亮

EK79007 的厂商初始化命令参考 Espressif Apache-2.0 `esp_lcd_ek79007` 组件，并保存在本项目中，后续可独立维护。
