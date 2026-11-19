<div align="center">
  <h1>ESP32-P4-WIFI6-DB-Touch-LCD-7C</h1>
  <p><strong>搭载 ESP32-C5 Wi-Fi 6 协处理器，配备 7 英寸 1024 × 600 MIPI-DSI 电容触摸屏的 ESP32-P4 开发板</strong></p>
  <p><a href="README.md">English</a> | <strong>简体中文</strong></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/actions/workflows/build.yml"><img alt="构建" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest"><img alt="最新版本" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C"></a>
    <a href="LICENSE"><img alt="许可证" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C"></a>
  </p>
  <p>
    <a href="https://www.waveshare.com/esp32-p4-wifi6-db-touch-lcd-7c.htm">🛒 产品</a> &middot;
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest">📦 固件</a> &middot;
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> &middot;
    <a href="examples/arduino/examples/">🔧 Arduino</a> &middot;
    <a href="https://docs.waveshare.net/ESP32-P4-WIFI6-DB-Touch-LCD-7C/">📖 文档</a>
  </p>
</div>

---

## ✨ 概述

ESP32-P4-WIFI6-DB-Touch-LCD-7C 以 ESP32-P4 为主控，通过 SDIO 连接 ESP32-C5 提供 Wi-Fi 6 和低功耗蓝牙，并配有 7 英寸 MIPI-DSI 电容触摸屏。本仓库提供 ESP-IDF 和 Arduino 示例、硬件测试控制台以及 ESP-Brookesia 综合应用。

| 功能 | 器件 / 接口 |
| --- | --- |
| MCU | ESP32-P4NRW32X，双核 RISC-V HP + LP Core |
| 无线 | ESP32-C5-MINI-1 协处理器，通过 4 位 SDIO 连接，支持双频 Wi-Fi 6 和 BLE |
| 存储器 | 32 MB NOR Flash + 32 MB PSRAM（封装内） |
| 显示 | 7 英寸 1024 × 600 LCD，采用 EK79007 驱动、2-lane MIPI-DSI 和 PWM 背光控制 |
| 触摸 | GT911 电容触摸控制器，I2C 接口 |
| 摄像头 | 2-lane MIPI-CSI 接口，使用 SCCB 控制。摄像头预览示例默认使用 OV5647。 |
| 音频 | ES8389 Audio Codec（I2C + I2S），带功放扬声器输出、两路差分模拟麦克风输入和耳麦插孔 |
| RTC | PCF85063ATL，I2C 接口 |
| 存储扩展 | 板载 microSD 卡槽，4 位 SDMMC 接口 |
| USB | USB 2.0 High-Speed 接口，另有 USB-to-UART0 用于烧录和串口日志 |
| 通信 | TWAI/CAN、RS232 和 RS485，均配有板载收发器 |
| 扩展 | P1（16 针）和 P2（13 针）排针，引出 13 个 ESP32-P4 GPIO、电源和通信信号 |

## 📦 快速开始

最快的体验方式是下载[最新版本](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest)中可直接烧录的固件包。

1. 下载所需示例和框架版本对应的 `*-combined.zip` 包。
2. 解压后用 `python -m pip install esptool` 安装 esptool。
3. 用 USB 线连接开发板。
4. Windows 运行 `flash_combined.bat COMx`，Linux 运行 `./flash_combined.sh /dev/ttyACM0`。
5. 烧录完成后若未自动重启，请手动复位。

## 🧪 示例

### ESP-IDF

| 示例 | 主要功能 |
| --- | --- |
| [board_check](examples/esp-idf/board_check/) | 芯片、Flash、PSRAM 和堆信息 |
| [nvs_counter](examples/esp-idf/nvs_counter/) | NVS 持久化计数器 |
| [generic_gpio](examples/esp-idf/generic_gpio/) | 通用 GPIO 输入输出 |
| [user_led](examples/esp-idf/user_led/) | 用户 LED 控制 |
| [mipi_dsi](examples/esp-idf/mipi_dsi/) | MIPI-DSI LCD 显示 |
| [lvgl_v8_demo](examples/esp-idf/lvgl_v8_demo/) | LVGL v8 显示性能测试 |
| [lvgl_v9_demo](examples/esp-idf/lvgl_v9_demo/) | LVGL v9 显示性能测试 |
| [sdmmc](examples/esp-idf/sdmmc/) | microSD 卡读写 |
| [uart_rs232](examples/esp-idf/uart_rs232/) | RS232 通信 |
| [uart_rs485](examples/esp-idf/uart_rs485/) | RS485 通信 |
| [i2c_console](examples/esp-idf/i2c_console/) | I2C 扫描和控制台工具 |
| [wifi_sta](examples/esp-idf/wifi_sta/) | 通过 ESP-Hosted 连接 Wi-Fi 热点 |
| [rtc](examples/esp-idf/rtc/) | PCF85063A 实时时钟 |
| [i2s_codec](examples/esp-idf/i2s_codec/) | I2S 编解码器音频 |
| [mipi_csi](examples/esp-idf/mipi_csi/) | MIPI-CSI 摄像头采集和显示 |
| [twai_sender](examples/esp-idf/twai_sender/) | TWAI/CAN 报文发送 |
| [twai_listen_only](examples/esp-idf/twai_listen_only/) | TWAI/CAN 只听接收 |
| [usb_extend_screen](examples/esp-idf/usb_extend_screen/) | USB 扩展屏 |
| [bleprph_gatt](examples/esp-idf/bleprph_gatt/) | ESP-Hosted NimBLE GATT 服务 |
| [hw_console](examples/esp-idf/hw_console/) | 串口控制台硬件测试固件 |

### Arduino

请先阅读[板卡文件说明](examples/arduino/examples/esp32/README_ZH.md)，然后在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C`。

| 示例 | 主要功能 |
| --- | --- |
| [board_check](examples/arduino/examples/board_check/) | 芯片、Flash、PSRAM 和内存信息 |
| [gpio](examples/arduino/examples/gpio/) | GPIO 和用户 LED 控制 |
| [i2c](examples/arduino/examples/i2c/) | I2C 设备扫描 |
| [i2s](examples/arduino/examples/i2s/) | ES8389 麦克风到扬声器回环 |
| [mipi_csi](examples/arduino/examples/mipi_csi/) | 摄像头画面采集和显示 |
| [mipi_dsi](examples/arduino/examples/mipi_dsi/) | LCD 初始化和测试色条 |
| [rtc](examples/arduino/examples/rtc/) | PCF85063A 时间读写 |
| [sdmmc](examples/arduino/examples/sdmmc/) | microSD 文件读写 |
| [twai](examples/arduino/examples/twai/) | TWAI/CAN 报文接收 |
| [uart/rs232](examples/arduino/examples/uart/rs232/) | USB 串口与 RS232 双向桥接 |
| [uart/rs485](examples/arduino/examples/uart/rs485/) | USB 串口与 RS485 半双工桥接 |

### 综合应用

[`examples/factory`](examples/factory/) 是 ESP-Brookesia 综合应用，包含显示、触摸、音频、Wi-Fi 和小智等功能。

### ESP32-P4 与 ESP32-C5 无线

ESP32-P4 本身不带 Wi-Fi 和蓝牙。板载 ESP32-C5 通过 4 位 SDIO 与其连接，提供 Wi-Fi 6 和低功耗蓝牙。使用无线功能需要 P4 主机应用，并且 C5 协处理器运行兼容的固件；烧录 P4 应用不会更新 C5 固件，两侧的 ESP-Hosted 配置必须保持兼容。详见 [wifi_sta](examples/esp-idf/wifi_sta/) 和 [bleprph_gatt](examples/esp-idf/bleprph_gatt/)。

## 🛠️ 开发环境

| 开发框架 | 版本 | 示例数 |
| --- | --- | ---: |
| ESP-IDF | `v5.5.6`、`v6.1` | 20 |
| Arduino-ESP32 | `3.3.12` | 11 |

## 📚 资料

- [硬件参考](docs/hw-reference/USER_GUIDE_ZH.md)
- [产品页面](https://www.waveshare.com/esp32-p4-wifi6-db-touch-lcd-7c.htm)
- [Wiki](https://docs.waveshare.net/ESP32-P4-WIFI6-DB-Touch-LCD-7C/)

## 🤝 支持

- 示例或仓库问题：[提交 issue](https://gitee.com/waveshare/ESP32-P4-WIFI6-DB-Touch-LCD-7C/issues)。
- 产品使用、硬件或订单问题：[技术支持](https://docs.waveshare.net/ESP32-P4-WIFI6-DB-Touch-LCD-7C/Technical-Support/)。

## 📄 许可证

Apache License 2.0，详见 [LICENSE](LICENSE)。
