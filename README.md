<div align="center">
  <h1>ESP32-P4-WIFI6-DB-Touch-LCD-7C</h1>
  <p><strong>ESP32-P4 development board with an ESP32-C5 Wi-Fi 6 coprocessor and a 7-inch 1024 × 600 MIPI-DSI capacitive touch display</strong></p>
  <p><strong>English</strong> | <a href="README_ZH.md">简体中文</a></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/actions/workflows/build.yml"><img alt="Build" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest"><img alt="Latest Release" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C"></a>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C"></a>
  </p>
  <p>
    <a href="https://www.waveshare.com/esp32-p4-wifi6-db-touch-lcd-7c.htm">🛒 Product</a> &middot;
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest">📦 Firmware</a> &middot;
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> &middot;
    <a href="examples/arduino/examples/">🔧 Arduino</a> &middot;
    <a href="https://docs.waveshare.com/ESP32-P4-WIFI6-DB-Touch-LCD-7C">📖 Documentation</a>
  </p>
</div>

---

## ✨ Overview

The ESP32-P4-WIFI6-DB-Touch-LCD-7C pairs an ESP32-P4 host with an ESP32-C5 that provides Wi-Fi 6 and Bluetooth LE over SDIO, and a 7-inch MIPI-DSI capacitive touch display. This repository provides ESP-IDF and Arduino examples, a hardware test console, and an ESP-Brookesia application.

| Feature | Device / interface |
| --- | --- |
| MCU | ESP32-P4NRW32X with dual high-performance RISC-V cores and a low-power core |
| Wireless | ESP32-C5-MINI-1 coprocessor with dual-band Wi-Fi 6 and Bluetooth LE over 4-bit SDIO |
| Memory | 32 MB NOR Flash and 32 MB in-package PSRAM |
| Display | 7-inch 1024 × 600 LCD with EK79007 driver, 2-lane MIPI-DSI and PWM backlight control |
| Touch | GT911 capacitive touch controller over I2C |
| Camera | 2-lane MIPI-CSI connector with SCCB control. The camera preview example uses OV5647 by default. |
| Audio | ES8389 codec over I2C and I2S, with amplified speaker output, two differential analog mic inputs and a headset jack |
| RTC | PCF85063ATL over I2C |
| Storage | On-board microSD slot over 4-bit SDMMC |
| USB | USB 2.0 High-Speed port and USB-to-UART0 bridge for flashing and serial logs |
| Communication | TWAI/CAN, RS232 and RS485 with on-board transceivers |
| Expansion | P1 (16-pin) and P2 (13-pin) headers exposing 13 ESP32-P4 GPIOs and power and communication signals |

## 📦 Quick Start

The fastest way to try an example is a ready-to-flash package from the
[latest release](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/releases/latest).

1. Download the `*-combined.zip` package for the example and framework version you need.
2. Extract the archive and install esptool with `python -m pip install esptool`.
3. Connect the board over USB.
4. Run `flash_combined.bat COMx` on Windows or `./flash_combined.sh /dev/ttyACM0` on Linux.
5. Reset the board if it does not restart automatically.

## 🧪 Examples

### ESP-IDF

| Example | Main feature |
| --- | --- |
| [board_check](examples/esp-idf/board_check/) | Chip, Flash, PSRAM and heap information |
| [nvs_counter](examples/esp-idf/nvs_counter/) | NVS persistent counter |
| [generic_gpio](examples/esp-idf/generic_gpio/) | General-purpose GPIO input and output |
| [user_led](examples/esp-idf/user_led/) | User LED control |
| [mipi_dsi](examples/esp-idf/mipi_dsi/) | MIPI-DSI LCD display |
| [lvgl_v8_demo](examples/esp-idf/lvgl_v8_demo/) | LVGL v8 display benchmark |
| [lvgl_v9_demo](examples/esp-idf/lvgl_v9_demo/) | LVGL v9 display benchmark |
| [sdmmc](examples/esp-idf/sdmmc/) | microSD card access |
| [uart_rs232](examples/esp-idf/uart_rs232/) | RS232 communication |
| [uart_rs485](examples/esp-idf/uart_rs485/) | RS485 communication |
| [i2c_console](examples/esp-idf/i2c_console/) | I2C scanning and console tools |
| [wifi_sta](examples/esp-idf/wifi_sta/) | Connect to Wi-Fi via ESP-Hosted |
| [rtc](examples/esp-idf/rtc/) | PCF85063A real-time clock |
| [i2s_codec](examples/esp-idf/i2s_codec/) | I2S codec audio |
| [mipi_csi](examples/esp-idf/mipi_csi/) | MIPI-CSI camera capture and display |
| [twai_sender](examples/esp-idf/twai_sender/) | TWAI/CAN frame transmission |
| [twai_listen_only](examples/esp-idf/twai_listen_only/) | TWAI/CAN listen-only reception |
| [usb_extend_screen](examples/esp-idf/usb_extend_screen/) | USB extended display |
| [bleprph_gatt](examples/esp-idf/bleprph_gatt/) | ESP-Hosted NimBLE GATT server |
| [hw_console](examples/esp-idf/hw_console/) | Serial-console hardware test firmware |

### Arduino

After installing Arduino-ESP32 `3.3.12`, select `ESP32P4 Dev Module` in the Arduino IDE (FQBN: `esp32:esp32:esp32p4:PSRAM=enabled,PartitionScheme=huge_app`), with **PSRAM** set to `Enabled` and **Partition Scheme** set to `Huge APP (3MB No OTA/1MB SPIFFS)`.

| Example | Main feature |
| --- | --- |
| [board_check](examples/arduino/examples/board_check/) | Chip, Flash, PSRAM and memory information |
| [gpio](examples/arduino/examples/gpio/) | GPIO and user LED control |
| [i2c](examples/arduino/examples/i2c/) | I2C device scanning |
| [i2s](examples/arduino/examples/i2s/) | ES8389 microphone-to-speaker loopback |
| [mipi_csi](examples/arduino/examples/mipi_csi/) | Camera capture and display |
| [mipi_dsi](examples/arduino/examples/mipi_dsi/) | LCD initialization and color bars |
| [rtc](examples/arduino/examples/rtc/) | PCF85063A time access |
| [sdmmc](examples/arduino/examples/sdmmc/) | microSD file access |
| [twai](examples/arduino/examples/twai/) | TWAI/CAN reception |
| [rs232](examples/arduino/examples/rs232/) | USB serial to RS232 bridge |
| [rs485](examples/arduino/examples/rs485/) | USB serial to RS485 half-duplex bridge |

### Application

[`examples/factory`](examples/factory/) contains the integrated ESP-Brookesia application with display, touch, audio, Wi-Fi and Xiaozhi features.

### ESP32-P4 and ESP32-C5 Wireless

The ESP32-P4 does not have built-in Wi-Fi or Bluetooth. The on-board ESP32-C5 connects over 4-bit SDIO and provides Wi-Fi 6 and Bluetooth LE. Wireless operation requires a P4 host application and compatible firmware on the C5 coprocessor; flashing the P4 application does not update the C5 firmware. Keep the ESP-Hosted configurations on both sides compatible. See [wifi_sta](examples/esp-idf/wifi_sta/) and [bleprph_gatt](examples/esp-idf/bleprph_gatt/).

## 🛠️ Development Environment

| Framework | Version | Examples |
| --- | --- | ---: |
| ESP-IDF | `v5.5.6`, `v6.1` | 20 |
| Arduino-ESP32 | `3.3.12` | 11 |

## 📚 Resources

- [Hardware reference](docs/hw-reference/USER_GUIDE.md)
- [Product page](https://www.waveshare.com/esp32-p4-wifi6-db-touch-lcd-7c.htm)
- [Wiki](https://docs.waveshare.com/ESP32-P4-WIFI6-DB-Touch-LCD-7C)

## 🤝 Support

- Example or repository problem: [open an issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-DB-Touch-LCD-7C/issues/new/choose).
- Product usage, hardware or order questions: [Waveshare support](https://service.waveshare.com/).

## 📄 License

Apache License 2.0. See [LICENSE](LICENSE).
