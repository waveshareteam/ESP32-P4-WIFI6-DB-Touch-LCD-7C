# ES8389 / ES8311 I2S Codec

[English Version](./README.md)

本示例通过 I2S 持续播放 PCM，或采集麦克风数据后回放。ESP32-P4
的仓库默认配置启用本地 BSP，使用 BSP 提供的 ES8389 播放和录音设备；关闭
BSP 后仍保留直接连接 ES8311 的参考路径。

## 硬件要求

- 默认 BSP/ES8389 路径使用 ESP32-P4-WIFI6-DB-TOUCH-LCD-10.1 开发板。
- 播放需要扬声器或耳机。
- Echo 模式需要麦克风输入。

## 构建和烧录

```powershell
cd examples\esp_idf\10_i2s_codec
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```

将 `COMx` 替换为实际串口号。

## 配置

在 **Example Configuration** 中：

- 选择 `music`，循环播放内置的 `main/canon.pcm`。
- 选择 `echo`，读取麦克风 PCM 并写回输出。
- 通过 **Voice volume** 设置 0～100 的输出音量。
- 本开发板保持 **Enable Board Support Package (BSP) support** 开启。

Kconfig 默认选择 `music`，`sdkconfig.defaults` 默认启用 BSP。Music 和 Echo
都有 BSP 实现路径。麦克风增益菜单只控制非 BSP 的 ES8311 路径；当前 BSP
辅助组件把 ES8389 输入增益设为 24 dB。

BSP 会把 I2S RX/TX 初始化为双声道。打开 ES8389 录音设备后，调用
`bsp_audio_codec_configure_microphone()` 配置两路差分 MIC：0x72 和 0x73
选择差分输入路径，并对两路应用相同的 PGA 增益。后续修改麦克风增益时，
也要再次调用这个 BSP 接口。

当前 ESP32-P4 I2C/I2S 引脚：

| 信号 | GPIO |
| --- | ---: |
| I2C SDA | 7 |
| I2C SCL | 8 |
| I2S MCLK | 13 |
| I2S BCLK | 12 |
| I2S WS | 10 |
| I2S DOUT | 9 |
| I2S DIN | 11 |
| 功放使能 | 53 |

## 预期行为

- BSP 路径启动时打印 `Using ESP32-P4 BSP for I2S and codec configuration`。
- 初始化成功后打印 `i2s driver init success` 和
  `es8311 codec init success`。第二条是遗留的函数/日志名称，ESP32-P4 BSP
  路径实际初始化的是 ES8389。
- Music 模式会持续打印写入的 PCM 字节数。
- Echo 模式先打印 `[echo] Echo start`，然后持续采集并回放 PCM。
- Echo 模式每秒打印一次 `[echo] captured peak: left=..., right=... (PCM16)`。

初始化日志只能证明软件路径已经打开，不能证明左右声道存在有效麦克风数据，也不能证明模拟输出已经有声。

## 排障

- 检查 Codec 供电、MCLK/BCLK/WS 波形、I2C 控制、I2S 接线、功放使能和当前扬声器/耳机路由。
- Echo 模式先分别统计左右声道 PCM 的 peak 或 RMS，再判断 Codec 或模拟硬件故障。
- 关闭 BSP 后，参考路径使用 ES8311，不是本板的 ES8389。
- 使用 [02_i2c_console_test](../02_i2c_console_test/) 确认 GPIO7/GPIO8
  上的目标 Codec 能正常应答。

本文已按源码完成静态核对；音频播放和录音仍为待实测。
