# BSP: ESP32-P4-WIFI6-DB

## Overview

This component provides the board support package for ESP32-P4-WIFI6-DB.
The current manifest version is `0.0.1`.

Supported board interfaces:

- SD card over 4-bit SDMMC.
- ES8389 playback and two differential analog microphone inputs.
- Shared I2C bus for the ES8389, PCF85063A RTC, GT911, and camera SCCB.
- EK79007 1024x600 landscape MIPI-DSI display over two data lanes.
- GT911 capacitive touch controller.
- MIPI-CSI camera through `esp_video`.
- USB Host.
- PCF85063A RTC.
- Two active-low user LEDs on GPIO2 and GPIO3.
- Expansion-header GPIO enumeration.

The LCD reset GPIO is GPIO31. The GT911 reset GPIO is GPIO23, while its
interrupt GPIO is not connected. The LEDC backlight output is GPIO30. The
audio power amplifier is enabled by GPIO53.

## Pin Assignment

### I2C

| Signal | GPIO |
| --- | ---: |
| SDA | 7 |
| SCL | 8 |

The ES8389, PCF85063A RTC, GT911, and camera SCCB bus share this I2C bus. The
LCD backlight uses LEDC instead of this bus.

### ES8389

| ES8389 signal | ESP32-P4 signal | GPIO |
| --- | --- | ---: |
| MCLK | I2S MCLK | 13 |
| SCLK | I2S BCLK | 12 |
| LRCK | I2S WS | 10 |
| DSDIN | I2S DOUT | 9 |
| ASDOUT | I2S DIN | 11 |
| Power amplifier enable | GPIO | 53 |

`bsp_audio_codec_speaker_init()` and
`bsp_audio_codec_microphone_init()` both create ES8389 codec devices. The
microphone path uses the left and right differential input paths.
After opening the microphone device, call
`bsp_audio_codec_configure_microphone()` to apply the same gain to both ADC
channels and restore the board's input routing.

### SD Card

| Signal | GPIO |
| --- | ---: |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |
| CMD | 44 |
| CLK | 43 |
| Power enable | 45, active low |

`bsp_sdcard_mount()` enables the card through GPIO45 before starting SDMMC
initialization. GPIO45 is active low. On ESP32-P4, SDMMC IO voltage is
controlled through on-chip LDO channel 4. If mounting fails, the current code
turns the card power off and releases the LDO; `bsp_sdcard_unmount()` also
turns the card power off. The BSP enables the SDMMC internal pull-ups;
external pull-ups are still required for normal signal integrity. No fixed
power-cycle delay is part of the current implementation.

### Display

The board uses the EK79007 1024x600 landscape panel. It has two MIPI-DSI data
lanes and a lane bit rate of 1000 Mbps. The LCD reset pin is GPIO31.

Backlight brightness is controlled by LEDC:

| Item | Value |
| --- | --- |
| GPIO | `BSP_LCD_BACKLIGHT` = GPIO30 (internal, not an expansion-header GPIO) |
| Mode | `LEDC_LOW_SPEED_MODE` |
| Timer/channel | `LEDC_TIMER_1` / `LEDC_CHANNEL_1` |
| Resolution/frequency | 10-bit / 5 kHz |

`bsp_display_new()` initializes the panel with the backlight off.
`bsp_display_brightness_set()` accepts `0..100` percent and converts the value
to LEDC duty. It initializes the LEDC channel on demand.

### User LEDs

| LED | GPIO | Active level |
| --- | ---: | ---: |
| `BSP_LED_USER1` / `BSP_USER1_LED` | 2 | Low |
| `BSP_LED_USER2` / `BSP_USER2_LED` | 3 | Low |

Call `bsp_led_init()` to configure both LEDs in the off state. Use
`bsp_led_set(BSP_LED_USER1, true)` or `bsp_led_set(BSP_LED_USER2, true)` to
turn on an individual LED. These GPIOs are not part of the expansion-header
array.

### RTC

The PCF85063A is present at `BSP_RTC_I2C_ADDRESS` on the shared GPIO7/GPIO8
I2C bus. `bsp_rtc_init()` preserves a valid retained time and sets the firmware
build time when the RTC contents are invalid. Use `bsp_rtc_get_time()` and
`bsp_rtc_set_time()` for normal access, or `bsp_rtc_set_build_time()` to force
the build timestamp.

### Touch

The GT911 uses the shared I2C bus on GPIO7/GPIO8. Its reset GPIO is GPIO23 and
its interrupt GPIO is `GPIO_NUM_NC`, so the driver polls the controller.
`bsp_touch_new()` tries both valid GT911 addresses, `0x5D` and `0x14`. Touch
coordinates use the EK79007 1024x600 display area.

`bsp_display_start()` and `bsp_display_start_with_config()` automatically
create the EK79007 display, create the GT911 device, and register it with the
LVGL adapter.

`bsp_touch_new()` creates and probes the GT911 I2C panel IO, resets the
controller, then automatically applies the reference GT911 configuration
before creating the touch controller. The API updates the GT911 configuration
block and verifies it by reading the block back; it does not replace the
GT911 controller program image.

### Expansion Header

`bsp_get_header_gpios()` returns the following BSP-owned, read-only array:

```text
32, 33, 49, 50, 52, 51, 48, 47, 46, 36, 25, 24, 5, 4
```

There are 14 GPIOs in the array. GPIO24 and GPIO25 are also exposed through
the `BSP_USB_N` and `BSP_USB_P` macros. They remain in the header array, so
applications using USB should reserve those two entries.

## Configuration

Use `menuconfig` to configure:

- I2C controller and 100/400 kHz bus speed.
- I2S controller.
- RGB565 or RGB888 display format.
- Number of EK79007 MIPI-DPI frame buffers.
- SD card and SPIFFS mount options.

## Basic Usage

```c
#include "bsp/esp-bsp.h"

ESP_ERROR_CHECK(bsp_i2c_init());
ESP_ERROR_CHECK(bsp_rtc_init());
ESP_ERROR_CHECK(bsp_led_init());
ESP_ERROR_CHECK(bsp_led_set(BSP_LED_USER1, true));

esp_codec_dev_handle_t playback = bsp_audio_codec_speaker_init();
esp_codec_dev_handle_t microphone = bsp_audio_codec_microphone_init();

pcf85063a_datetime_t now = {0};
ESP_ERROR_CHECK(bsp_rtc_get_time(&now));

const bsp_display_config_t display_config = {
    .dsi_bus = {
        .phy_clk_src = 0,
        .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
    },
    .num_fbs = 0,
};
esp_lcd_panel_handle_t panel = NULL;
esp_lcd_panel_io_handle_t panel_io = NULL;
ESP_ERROR_CHECK(bsp_display_new(&display_config, &panel, &panel_io));
ESP_ERROR_CHECK(bsp_display_brightness_set(50));

esp_lcd_touch_handle_t touch = NULL;
ESP_ERROR_CHECK(bsp_touch_new(NULL, &touch));
```

The EK79007 display, LEDC backlight, GT911 touch, audio, SD card, RTC, USB,
CSI, and GPIO behavior still require feature-specific hardware checks.
