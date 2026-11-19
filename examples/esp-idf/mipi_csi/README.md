# MIPI-CSI Camera Preview

[中文版本](./README_ZH.md)

## What It Does

The example initializes `display_lcd`, `lcd_brightness`, and `camera` by name through `esp_board_manager`. `esp_video_capture_service` supplies RGB565 frames. `esp_video_render` uses the direct LCD backend by default. Enable `CONFIG_ESP_VIDEO_RENDER_LVGL_BACKEND_SUPPORT` to use the optional LVGL backend, with `esp_lvgl_port` managing DSI RGB565 double-buffered direct mode. The application does not manage PPA, frame copies, LCD refresh callbacks, or frame buffer switching; those operations belong to the components.

## Hardware Required

- ESP32-P4-WIFI6-DB Touch LCD 7C with a 1024 x 600 EK79007 MIPI-DSI panel.
- An `esp_video` supported MIPI-CSI camera. The board defaults select OV5647 RAW8 800 x 800 at 50 FPS.
- LCD, backlight, camera SCCB/I2C, and shared MIPI LDO parameters come from the Board Manager configuration.
- Run `idf.py bmgr` to generate `components/gen_bmgr_codes` for `esp32_p4_wifi6_db_touch_lcd_7c` before building. Rerun it after changing board YAML files.
- The application supports unrotated, unmirrored DSI RGB565_LE panels. Dimensions are read from the LCD configuration and the pixel format is validated. Before LCD initialization, the application overrides the frame buffer count to two, for LVGL direct mode and compatibility with the `esp_video_render` 1.1 direct LCD backend limit; board YAML files are unchanged.
- Touch, audio, and filesystems are not initialized by this example.

## How to Run

Requires ESP-IDF v5.5 or later. Run manually:

```powershell
cd examples/esp-idf/mipi_csi
idf.py set-target esp32p4
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py build
idf.py -p PORT flash monitor
```

Select the sensor and output format in the `Espressif Camera Sensors` menu of `idf.py menuconfig`. The generated board configuration supplies the default format.

## Expected Result

The backlight turns on and the LCD shows live camera frames. The capture service manages three camera buffers to leave capture headroom while a frame is held for rendering. Each frame is passed to `esp_video_render_stream_write()` and then released. The single render stream runs synchronously with its extra frame cache disabled. Capture frames are released only after `stream_write()` returns; asynchronous rendering is not enabled.

`gmf_video` supplies the `vid_ppa` hardware scaling element. The complete image is scaled to fit the screen while preserving its aspect ratio, centered over a black background. With the default 800 x 800 camera and 1024 x 600 panel, the preview is 600 x 600. No application-level mirroring is applied. Every two seconds, the application logs frame submission FPS, average frame acquisition wait, and average render time; the configured FPS is not a measured display rate.

The LVGL backend is disabled by default in `sdkconfig.defaults`. Text widgets are disabled and base widget support remains enabled. The optional LVGL path is preset to RGB565 with a 10 ms refresh period; when enabled, it creates a video image object without extra controls. The `esp_video_render` 1.1.0 manifest checks the text-widget option to resolve FreeType; disabling its widget parent can omit that symbol from `sdkconfig.json`, causing `Missing required kconfig option after retry`. `esp_video_render` can still use PPA internally for composition on ESP32-P4; removing application-level PPA code does not disable that internal acceleration.

## Troubleshooting

- Run [04_mipi_dsi](../mipi_dsi/) first to check the display path.
- For camera initialization or open failures, check the FPC, power, selected sensor, and board I2C configuration.
- Enable PSRAM. Buffer allocation failures are reported immediately.
- For `ESP_ERR_NOT_SUPPORTED`, check the LCD interface, RGB565 format, and orientation. LCD initialization also requires the two-buffer runtime override to succeed.

With the LVGL backend, the `Preview` FPS still measures application frame submissions, not physical LCD refreshes. Compare visible smoothness as well as the log.
