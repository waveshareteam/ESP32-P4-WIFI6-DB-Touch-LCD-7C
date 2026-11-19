# I2S Codec 音频示例

[English Version](./README.md)

## 功能

本示例在 Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C 上使用 ES8389 Codec，演示内置 PCM 循环播放和麦克风实时回放。音频仅通过 `esp_board_manager` 和 `esp_codec_dev` 完成，不使用 player service 或解码器。

| 组件 | 用途 |
| --- | --- |
| `esp_board_manager` | 按板级配置初始化音频 ADC、DAC、I2S、I2C 和功放 |
| `esp_codec_dev` | 向 DAC 写入 PCM、从 ADC 读取麦克风 PCM，设置音量和麦克风增益 |

## 硬件需求

ESP32-P4-WIFI6-DB-Touch-LCD-7C，使用板载 ES8389、麦克风和扬声器。

## 怎么运行

在仓库根目录执行：

```powershell
cd examples/esp-idf/i2s_codec
idf.py set-target esp32p4
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```

`bmgr` 命令使用托管板包中的板级定义生成 `components/gen_bmgr_codes`。将 `COMx` 替换为实际串口号。项目使用 32 MiB Flash 配置和 `partitions.csv` 中的 4 MiB `factory` 分区；复用已有 `sdkconfig` 时，请确认选中该自定义分区表。

### 模式与配置

在 `menuconfig → Example Configuration → Example mode` 中选择模式：

| 模式 | 使用方法 |
| --- | --- |
| **Embedded PCM loop**（默认） | 循环播放编译进固件的 `main/canon.pcm`，无需 SD 卡 |
| **echo** | 对着麦克风说话，由扬声器实时回放；不启用 AEC 回声消除 |

**Voice volume** 设置播放音量（0～100，默认 60）；Echo 模式下的 **Set MIC gain** 设置麦克风模拟增益（0～42 dB，步进 6 dB，默认 24 dB）。

## 预期现象

启动后，Board Manager 先初始化当前模式需要的设备。然后通过 `esp_codec_dev_open()` 打开 Codec，并启动直接读写 PCM 的任务：

| 模式 | 音频路径 |
| --- | --- |
| 内置 PCM | 内置 PCM 数据 → `esp_codec_dev_write()` → ES8389 DAC → 扬声器 |
| Echo | 麦克风 → ES8389 ADC → `esp_codec_dev_read()` → `esp_codec_dev_write()` → ES8389 DAC → 扬声器 |

两种模式均使用 16 kHz、双声道、16 位 PCM。

## 常见问题

暂无。
