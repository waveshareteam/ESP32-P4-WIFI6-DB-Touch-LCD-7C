# Waveshare ESP32-P4-WIFI6-DB Touch LCD 7C

[中文版](./README.md)

This repository contains examples and firmware for the Waveshare
ESP32-P4-WIFI6-DB Touch LCD 7C development board. It includes Arduino
examples, ESP-IDF examples, a board support package (BSP), and an integrated
ESP-Brookesia application example.

## On-board Peripherals

| Peripheral | Description |
| --- | --- |
| Main controller | ESP32-P4 for application, display, camera, audio, and peripheral control |
| Wireless | ESP32-C5 wireless co-processor connected to the ESP32-P4 through ESP-Hosted SDIO, providing Wi-Fi 6 and Bluetooth/BLE capabilities |
| LCD | EK79007 1024×600 landscape MIPI-DSI LCD with two data lanes |
| Touch | GT911 capacitive touch controller on the shared I2C bus |
| Camera | MIPI-CSI camera interface; the BSP supports sensors such as OV5647 and SC2336 |
| Audio | ES8389 codec with speaker playback and two differential analog microphone inputs |
| RTC | PCF85063A real-time clock at I2C address `0x51` |
| Storage | microSD card through a 4-bit SDMMC interface |
| USB | USB interface and USB Host support; USB differential signals are GPIO24/GPIO25 |
| CAN | On-board TWAI/CAN transceiver |
| Serial transceivers | On-board RS232 and RS485 transceivers |
| User LEDs | Two user LEDs on GPIO2 and GPIO3; active low |
| Expansion header | 14 expansion GPIOs: `32, 33, 49, 50, 52, 51, 48, 47, 46, 36, 25, 24, 5, 4` |

## Key Pins

| Function | Pins |
| --- | --- |
| Shared I2C | SDA GPIO7, SCL GPIO8 |
| ES8389 I2S | MCLK GPIO13, BCLK GPIO12, WS GPIO10, DOUT GPIO9, DIN GPIO11 |
| Audio amplifier enable | GPIO53 |
| LCD | Reset GPIO31, backlight GPIO30 |
| GT911 | Reset GPIO23, interrupt not connected |
| SDMMC | CLK GPIO43, CMD GPIO44, D0-D3 GPIO39/40/41/42, power GPIO45 (active low) |
| TWAI/CAN | TX GPIO22, RX GPIO21 |
| RS485 | TX GPIO26, RX GPIO27, direction control GPIO34 |
| RS232 | TX GPIO28, RX GPIO29 |
| ESP-Hosted SDIO | CLK GPIO18, CMD GPIO19, D0-D3 GPIO14/15/16/17, reset GPIO54 |

ES8389, PCF85063A, GT911, and the camera SCCB interface share the GPIO7/GPIO8
I2C bus. Do not configure these pins as general-purpose GPIOs while using
these peripherals.

## Directory Structure

| Directory | Purpose |
| --- | --- |
| `asserts/` | Reserved directory; currently empty |
| `hardware/` | Reserved directory for hardware documentation; currently empty |
| `examples/Arduino/` | Arduino IDE examples; each subdirectory is an independent example |
| `examples/esp_idf/` | ESP-IDF peripheral and board-function test examples, organized by number |
| `firmware/esp32_p4_wifi6_db_touch_lcd_7c/` | ESP-IDF board support package (BSP) for the display, touch, audio, RTC, SD card, USB, and camera interfaces |
| `firmware/brookesia_test/` | Complete ESP-Brookesia/Xiaozhi application example using the display, touch, audio, Wi-Fi, RTC, and camera features |

## Arduino Examples

Select `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C` in the Arduino IDE, then
open and upload each example in `examples/Arduino/` independently.

| Directory | Purpose |
| --- | --- |
| `board_check` | Print chip, Flash, PSRAM, and memory information |
| `gpio` | Test GPIOs and user LEDs through serial commands |
| `i2c` | Scan I2C device addresses |
| `i2s` | ES8389 microphone-to-speaker I2S loopback |
| `mipi_csi` | Capture camera frames and display them on the LCD |
| `mipi_dsi` | Initialize the MIPI-DSI LCD and display test color bars |
| `rtc` | Read the PCF85063A RTC and initialize invalid time with the firmware build time |
| `sdmmc` | Mount the microSD card and test file read/write operations |
| `squareline_wifi_clock` | SquareLine/LVGL Wi-Fi clock interface |
| `twai` | Receive 500 kbit/s TWAI/CAN frames in listen-only mode |
| `uart/rs232` | Bidirectional bridge between the USB serial port and RS232 |
| `uart/rs485` | Half-duplex bidirectional bridge between the USB serial port and RS485 |

See the `README_CN.md` in each Arduino example directory for its usage steps.

## ESP-IDF Examples

Each numbered directory under `examples/esp_idf/` is an independent ESP-IDF
project:

| Directory | Purpose |
| --- | --- |
| `00_board_check` | Basic board information check |
| `01_nvs_counter` | NVS non-volatile counter |
| `02_gpio_input_test` | GPIO input and touch-input test |
| `03_led_test` | User LED test |
| `04_mipi_dsi_test` | MIPI-DSI display color-bar test |
| `05_sdmmc_test` | SDMMC card test |
| `06_uart_test` | UART loopback test |
| `07_i2c_console_test` | I2C console tools |
| `08_wifi_test` | ESP32-C5 ESP-Hosted Wi-Fi test |
| `09_bt_test` | ESP32-C5 Hosted Bluetooth/BLE test |
| `10_rtc_test` | PCF85063A RTC test |
| `11_i2s_codec` | ES8389/ES8311 I2S codec test |
| `12_mipi_csi_test` | MIPI-CSI camera display test |
| `13_twai_test` | TWAI/CAN test |
| `14_usb_extend_screen` | Use the board as a computer's USB extended display |

See `README.md` or `README_CN.md` in each ESP-IDF example directory for build,
flash, and configuration instructions.

## Firmware and BSP Relationship

`firmware/esp32_p4_wifi6_db_touch_lcd_7c/` is a reusable BSP component, not a
standalone application. ESP-IDF applications use this BSP to initialize the
board hardware.

`firmware/brookesia_test/` is a complete application project that uses the
local BSP for an integrated UI and feature test. The ESP32-C5 wireless
co-processor requires separately flashed, matching ESP-Hosted firmware;
flashing the ESP32-P4 application does not update the C5 firmware.
