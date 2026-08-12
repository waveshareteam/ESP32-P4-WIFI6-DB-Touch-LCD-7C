# ESP32-P4-WIFI6-DB BSP API

Include the complete board API with:

```c
#include "bsp/esp-bsp.h"
```

The board-specific header is `bsp/esp32_p4_wifi6_db_touch_lcd_7c.h`.

## Capabilities

| Capability | Macro | Value |
| --- | --- | ---: |
| Display | `BSP_CAPS_DISPLAY` | 1 |
| Touch | `BSP_CAPS_TOUCH` | 1 |
| Audio | `BSP_CAPS_AUDIO` | 1 |
| Speaker | `BSP_CAPS_AUDIO_SPEAKER` | 1 |
| Microphone | `BSP_CAPS_AUDIO_MIC` | 1 |
| SD card | `BSP_CAPS_SDCARD` | 1 |
| Camera | `BSP_CAPS_CAMERA` | 1 |
| RTC | `BSP_CAPS_RTC` | 1 |

## User LEDs

- `BSP_USER1_LED` is GPIO2 and `BSP_USER2_LED` is GPIO3.
- GPIO2 and GPIO3 are internal LED pins and are not part of the expansion-header array.
- Both LEDs are active low: level 0 turns an LED on and level 1 turns it off.
- `bsp_led_t` selects `BSP_LED_USER1` or `BSP_LED_USER2`.
- `bsp_led_init()` configures both GPIOs as outputs and starts with both LEDs
  off.
- `bsp_led_set(led, enabled)` controls the selected LED through the enum and
  initializes the LED GPIOs automatically if needed.
- `bsp_led_deinit()` turns both LEDs off and resets their GPIOs.

## Board and I2C

- `bsp_get_header_gpios()` returns 14 expansion-header GPIOs in this order:
  `32, 33, 49, 50, 52, 51, 48, 47, 46, 36, 25, 24, 5, 4`.
- GPIO24 and GPIO25 are also available as `BSP_USB_N` and `BSP_USB_P`. They
  remain in the returned header array for compatibility; applications using
  USB should reserve those two header entries.
- `bsp_i2c_init()` initializes GPIO7/GPIO8.
- `bsp_i2c_get_handle()` returns the shared master-bus handle.
- `bsp_i2c_deinit()` releases the shared bus when no device still owns it.

The ES8389, PCF85063A RTC, GT911 touch controller, and camera SCCB controller
share this bus. The display backlight is controlled by LEDC and does not use
this I2C bus.

## PCF85063A RTC

- `bsp_rtc_init()` initializes the RTC and preserves a valid retained time.
  If the RTC time is invalid, it sets the firmware build time.
- `bsp_rtc_set_time()` and `bsp_rtc_get_time()` set and read
  `pcf85063a_datetime_t` values.
- `bsp_rtc_set_build_time()` explicitly writes the firmware build timestamp.
- `bsp_rtc_get_handle()` returns the initialized PCF85063A device handle.
- `bsp_rtc_deinit()` removes the RTC device from the shared I2C bus.

## ES8389 Audio

- `bsp_audio_init()` creates full-duplex I2S channels.
- `bsp_audio_codec_speaker_init()` creates the ES8389 playback device.
- `bsp_audio_codec_microphone_init()` creates the ES8389 two-channel capture
  device for the board's differential microphones.
- `bsp_audio_codec_configure_microphone()` applies the same gain to both
  microphone channels and selects the differential input paths. Call it after
  opening the microphone device and after later gain changes. Open the device
  with `channel = 2` to receive both channels.

I2S pins are MCLK GPIO13, SCLK GPIO12, LRCK GPIO10, DSDIN GPIO9, and ASDOUT
GPIO11.

## Storage

- `bsp_sdcard_mount()` and `bsp_sdcard_unmount()` use the board's 4-bit SDMMC
  wiring.
- `bsp_sdcard_sdmmc_mount()` accepts caller-provided mount, host, or slot
  configuration.
- `bsp_sdcard_get_handle()` returns the mounted card.
- `bsp_spiffs_mount()` and `bsp_spiffs_unmount()` provide the optional
  internal SPIFFS helper.
- The SD power-enable GPIO is GPIO45 and is active low. SDMMC I/O voltage is
  controlled through on-chip LDO channel 4.
- The existing mount flow powers the card before SDMMC mounting, and powers it
  off when mounting fails or after unmounting. No additional power-cycle delay
  is part of the current BSP contract.

## MIPI-DSI Display

- The board uses the EK79007 1024x600 landscape panel over two MIPI-DSI data
  lanes at 1000 Mbps.
- `bsp_display_new()` and `bsp_display_new_with_handles()` create, reset, and
  initialize the EK79007 panel. The backlight remains off until explicitly
  enabled.
- `bsp_display_delete()` releases the panel, DSI bus, DSI PHY LDO, and
  LEDC backlight resources when they have been initialized.
- `bsp_display_brightness_init()` configures the LEDC backlight timer and
  channel on internal GPIO30 (`BSP_LCD_BACKLIGHT`), which is not part of the
  expansion-header array.
- `bsp_display_brightness_set()` accepts `0..100` percent and converts it to
  the configured LEDC duty. It initializes LEDC on demand.
- `bsp_display_brightness_get()` returns the last successfully configured
  percentage.
- `bsp_display_backlight_on()` and `bsp_display_backlight_off()` set 100% and
  0%.

## GT911 Touch

- `bsp_touch_new()` creates the GT911 on the shared GPIO7/GPIO8 I2C bus.
- Both GT911 addresses, `0x5D` and `0x14`, are tried.
- `bsp_touch_new()` creates and probes the I2C panel IO, resets the controller,
  then automatically applies and verifies the reference 186-byte GT911
  configuration for the 1024x600 panel before creating the touch controller.
- `BSP_LCD_TOUCH_RST` is GPIO23 and `BSP_LCD_TOUCH_INT` is `GPIO_NUM_NC`.
  Touch data is polled over I2C.
- Touch coordinates use the EK79007 1024x600 display area.
- `bsp_touch_delete()` releases the touch controller and its panel-I/O handle.

When LVGL support is enabled:

- `bsp_display_start()` initializes the display, GT911, and LVGL adapter.
- `bsp_display_start_with_config()` accepts rotation and tear-avoidance
  settings plus touch coordinate transforms.
- `bsp_display_stop()` stops LVGL and releases the display.
- `bsp_display_get_input_dev()` returns the registered LVGL touch input device.
- `bsp_display_lock()` and `bsp_display_unlock()` guard LVGL access.
- `bsp_display_set_dummy_draw()`,
  `bsp_display_get_free_frame_buffer()`, and
  `bsp_display_flush_frame_buffer()` support direct framebuffer producers.

## USB and CSI

- `BSP_USB_N` is GPIO24 and `BSP_USB_P` is GPIO25. These aliases do not remove
  the pins from `bsp_get_header_gpios()`.
- `bsp_usb_host_start()` and `bsp_usb_host_stop()` manage the USB Host
  library.
- `bsp_camera_start()` initializes the MIPI-CSI path with the shared I2C bus
  as the camera SCCB controller.
