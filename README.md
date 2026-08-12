# Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C

[English Version](./README_EN.md)

这是 Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C 开发板的示例和固件工程。
工程同时包含 Arduino 示例、ESP-IDF 示例、板级支持包（BSP）以及一个较完整的
ESP-Brookesia 应用示例。

## 板载外设

| 外设 | 说明 |
| --- | --- |
| 主控 | ESP32-P4，负责应用、显示、摄像头、音频和外设控制 |
| 无线 | ESP32-C5 无线协处理器，通过 ESP-Hosted SDIO 连接 ESP32-P4，提供 Wi-Fi 6 和 Bluetooth/BLE 能力 |
| LCD | EK79007，1024×600 横屏 MIPI-DSI LCD，双数据通道 |
| 触摸 | GT911 电容触摸控制器，使用共享 I2C 总线 |
| 摄像头 | MIPI-CSI 摄像头接口，BSP 支持 OV5647、SC2336 等传感器 |
| 音频 | ES8389 Codec，支持扬声器播放和两路差分模拟麦克风输入 |
| RTC | PCF85063A 实时时钟，I2C 地址为 `0x51` |
| 存储 | microSD 卡，4-bit SDMMC 接口 |
| USB | USB 接口和 USB Host 支持；USB 差分信号为 GPIO24/GPIO25 |
| CAN | 板载 TWAI/CAN 收发器 |
| 串口 | 板载 RS232 和 RS485 收发器 |
| 指示灯 | 两个用户 LED：GPIO2、GPIO3，低电平点亮 |
| 扩展排针 | 提供 14 个扩展 GPIO：`32, 33, 49, 50, 52, 51, 48, 47, 46, 36, 25, 24, 5, 4` |

## 关键引脚

| 功能 | 引脚 |
| --- | --- |
| 共享 I2C | SDA GPIO7，SCL GPIO8 |
| ES8389 I2S | MCLK GPIO13，BCLK GPIO12，WS GPIO10，DOUT GPIO9，DIN GPIO11 |
| 音频功放使能 | GPIO53 |
| LCD | 复位 GPIO31，背光 GPIO30 |
| GT911 | 复位 GPIO23，中断脚未连接 |
| SDMMC | CLK GPIO43，CMD GPIO44，D0-D3 GPIO39/40/41/42，供电 GPIO45（低有效） |
| TWAI/CAN | TX GPIO22，RX GPIO21 |
| RS485 | TX GPIO26，RX GPIO27，方向控制 GPIO34 |
| RS232 | TX GPIO28，RX GPIO29 |
| ESP-Hosted SDIO | CLK GPIO18，CMD GPIO19，D0-D3 GPIO14/15/16/17，复位 GPIO54 |

ES8389、PCF85063A、GT911 和摄像头 SCCB 共用 GPIO7/GPIO8 I2C 总线，使用这些
外设时不要把这两个 GPIO 当作普通 GPIO 重复配置。

## 目录说明

| 目录 | 用途 |
| --- | --- |
| `asserts/` | 预留目录，当前为空 |
| `hardware/` | 硬件资料预留目录，当前为空 |
| `examples/Arduino/` | Arduino IDE 示例，每个子目录都是一个独立示例 |
| `examples/esp_idf/` | ESP-IDF 外设和板级功能测试示例，按编号组织 |
| `firmware/esp32_p4_wifi6_db_touch_lcd_7c/` | ESP-IDF 板级支持包（BSP），提供显示、触摸、音频、RTC、SD 卡、USB、摄像头等接口 |
| `firmware/brookesia_test/` | 基于 ESP-Brookesia/Xiaozhi 的完整应用示例，综合使用显示、触摸、音频、Wi-Fi、RTC 和摄像头等功能 |

## Arduino 示例

`examples/Arduino/` 下的示例可在 Arduino IDE 中选择
`Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C` 后单独打开和上传。

| 目录 | 功能 |
| --- | --- |
| `board_check` | 打印芯片、Flash、PSRAM 和内存信息 |
| `gpio` | 通过串口命令测试 GPIO 和用户 LED |
| `i2c` | 扫描 I2C 设备地址 |
| `i2s` | ES8389 麦克风到扬声器的 I2S 回环 |
| `mipi_csi` | 采集摄像头画面并显示到 LCD |
| `mipi_dsi` | 初始化 MIPI-DSI LCD 并显示测试色条 |
| `rtc` | 读取 PCF85063A RTC，RTC 无效时使用编译时间初始化 |
| `sdmmc` | 挂载 microSD 并测试文件读写 |
| `squareline_wifi_clock` | SquareLine/LVGL Wi-Fi 时钟界面 |
| `twai` | 以监听模式接收 500 kbit/s TWAI/CAN 报文 |
| `uart/rs232` | USB 串口与 RS232 之间的双向桥接 |
| `uart/rs485` | USB 串口与 RS485 之间的半双工双向桥接 |

每个 Arduino 示例的具体操作步骤见对应目录中的 `README.md`。

## ESP-IDF 示例

`examples/esp_idf/` 中每个编号目录都是一个独立的 ESP-IDF 工程：

| 目录 | 功能 |
| --- | --- |
| `00_board_check` | 开发板基础信息检查 |
| `01_nvs_counter` | NVS 非易失计数器 |
| `02_gpio_input_test` | GPIO 输入和触摸输入测试 |
| `03_led_test` | 用户 LED 测试 |
| `04_mipi_dsi_test` | MIPI-DSI 显示色条测试 |
| `05_sdmmc_test` | SDMMC 卡测试 |
| `06_uart_test` | UART 回环测试 |
| `07_i2c_console_test` | I2C 控制台工具 |
| `08_wifi_test` | ESP32-C5 ESP-Hosted Wi-Fi 测试 |
| `09_bt_test` | ESP32-C5 Hosted Bluetooth/BLE 测试 |
| `10_rtc_test` | PCF85063A RTC 测试 |
| `11_i2s_codec` | ES8389/ES8311 I2S Codec 测试 |
| `12_mipi_csi_test` | MIPI-CSI 摄像头显示测试 |
| `13_twai_test` | TWAI/CAN 测试 |
| `14_usb_extend_screen` | 通过 USB 将开发板作为电脑扩展屏 |

ESP-IDF 示例的编译、烧录和配置方法见各目录下的 `README.md` 或
`README_CN.md`。

## 固件和 BSP 的关系

`firmware/esp32_p4_wifi6_db_touch_lcd_7c/` 是可复用的 BSP 组件，不是单独的
应用程序。ESP-IDF 应用通过该 BSP 初始化本开发板的硬件。

`firmware/brookesia_test/` 是一个完整应用工程，使用本地 BSP 运行综合 UI 和
功能测试。ESP32-C5 无线协处理器需要单独烧录匹配的 ESP-Hosted 固件；烧录
ESP32-P4 应用不会自动更新 C5 固件。
