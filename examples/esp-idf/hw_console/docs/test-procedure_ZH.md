# ESP32-P4-WIFI6-DB-Touch-LCD-7C 硬件验证操作手册

[English](test-procedure.md) | [简体中文](test-procedure_ZH.md)

本手册配套固件 `examples/esp-idf/hw_console`，用于逐项验证板载外设，并抓取关键信号波形。

## 约定

- 串口参数：115200 8N1，提示符 `waveshare>`。随时可输入 `help` 查看全部命令。
- 示例中 `waveshare>` 后面的内容是要输入的命令，其余行是预期输出，具体数值因板而异。
- 每节末尾的 **合格标准** 用于判定结果，**记录** 列出需要记下的测量值和现象。
- 后台任务：用 `jobs` 查看，用 `kill <id>` 或 `kill all` 停止。后台任务每 5 秒打印一行统计（`iter` / `err` / `MB/s`）。
- 抓波形：先用 `watch <命令>` 启动后台循环，接好探头完成测量后再 `kill`。所有探测点汇总在 [附录](#附录示波器探测点)。

## 测试流程总览

| 节 | 项目 | 方式 | 需要的物料 |
|---|---|---|---|
| [1](#1-系统信息与-nvs) | 系统信息、NVS | 自动 | — |
| [2](#2-i2c) | I2C 100 kHz / 400 kHz | 自动 + 波形 | 示波器 |
| [3](#3-sd-卡) | SD 卡 20 MHz / 40 MHz | 自动 + 波形 | microSD 卡、示波器 |
| [4](#4-usb-host) | USB Host | 自动 + 波形 | U 盘、示波器 |
| [5](#5-wi-fi) | Wi-Fi（ESP32-C5） | 半自动 | AP、PC（iperf 2） |
| [6](#display-backlight-touch) | 屏幕 / 背光 / 触摸 | 人工 | 屏幕 |
| [7](#7-摄像头) | 摄像头 | 人工 | 屏幕、摄像头 |
| [8](#8-音频) | 音频 | 人工 + 波形 | 喇叭、示波器、microSD 卡 |
| [9](#9-扩展排针-gpio) | 扩展排针 GPIO | 人工 | 万用表、示波器 |
| [10](#10-用户-led) | 用户 LED | 人工 | — |
| [11](#11-rs232) | RS232 | 人工 | USB 转 RS232 模块、PC |
| [12](#12-rs485) | RS485 | 人工 | USB 转 RS485 模块、PC |
| [13](#13-can) | CAN | 半自动 | USB-CAN 分析仪、120 Ω 终端电阻 |
| [14](#14-汇总报告) | 汇总报告 | 自动 | — |

建议顺序：先运行 `test all`（见第 14 节）完成全部自动项，再按第 2–13 节做波形测量和人工项，最后用 `test report` 查看汇总。

## 0. 测试前准备

### 物料

| 物料 | 要求 |
|---|---|
| 被测板 | 已烧录 hw_console 固件；Wi-Fi 测试还需要 ESP32-C5 已烧录 `examples/esp-idf/wifi_iperf/cp` |
| microSD 卡 | 支持 High Speed（Class 10 / UHS-I 均可），FAT32 格式，卡上数据会被覆盖 |
| U 盘 | FAT32 格式，容量不限，工作电流 ≤ 500 mA |
| Wi-Fi AP | 2.4 GHz 或 5 GHz，WPA2；PC 最好用网线接到同一路由器 |
| PC | 串口终端；iperf **2.x**（用 `iperf --version` 确认，不能用 iperf3） |
| 示波器 / 逻辑分析仪 | 带宽 ≥ 100 MHz（SD CLK 40 MHz） |
| 万用表 | 测量排针电平 |
| 屏幕 | Waveshare EK79007 1024×600 MIPI-DSI 屏，GT911 触摸 |
| 摄像头 | OV5647 MIPI-CSI 模组 |
| 喇叭 | 接板载功放输出 |
| 音频文件（可选） | SD 卡中放一个 mp3，用于 `aplay` 试听 |
| USB 转 RS232 模块 | 接电脑，测试 RS232 收发 |
| USB 转 RS485 模块 | 接电脑，测试 RS485 收发 |
| USB-CAN 分析仪 | 500 kbit/s，配 120 Ω 终端电阻 |

### 上电检查

上电后串口应输出：

```
ESP32-P4-WIFI6-DB-Touch-LCD-7C factory test console
Type 'help' for the command list. Nothing is initialised until you use it.
waveshare>
```

固件启动时不初始化任何外设，每个外设在第一次使用相关命令时才初始化。

## 1. 系统信息与 NVS

```
waveshare> uname -a
chip:      ESP32-P4 rev v1.0, 2 core(s)
idf:       v6.0.3
flash:     32 MB
psram:     32 MB
base mac:  xx:xx:xx:xx:xx:xx
waveshare> free
waveshare> nvs test
nvs ok, boot_count=1
```

**合格标准**：flash 为 32 MB，psram 容量正确，`nvs test` 输出 `nvs ok`。
**记录**：芯片版本、MAC 地址。

## 2. I2C

| 信号 | 引脚 |
|---|---|
| SDA | GPIO7 |
| SCL | GPIO8 |

总线上的设备：

| 设备 | 7-bit 地址 | 说明 |
|---|---|---|
| ES8389 音频编解码器 | `0x10` | Board Manager 中配置为 `0x20`，codec 驱动会右移一位后使用 |
| GT911 触摸 | `0x5d` 或 `0x14` | 需要接屏才能扫到 |

背光由 GPIO30 的 LEDC 输出控制，不在 I2C 总线上。

### 扫描

```
waveshare> i2cconfig --freq 100000
waveshare> i2cdetect
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
10: 10 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
...
50: -- -- -- -- -- -- -- -- -- -- -- -- -- 5d -- --
...
i2c.detect.100k: PASS (es8389@0x10:y gt911:y)
waveshare> i2cconfig --freq 400000
waveshare> i2cdetect
i2c.detect.400k: PASS (es8389@0x10:y gt911:y)
```

可选的寄存器抽查：`i2cget -c 0x10 -r 0x00`、`i2cdump -c 0x10`。

### 抓波形

```
waveshare> watch i2cget -c 0x10 -r 0x00
```

探 GPIO8（SCL）和 GPIO7（SDA）。分别在 100 kHz 和 400 kHz 下测量 SCL 周期（应为 10 µs / 2.5 µs）、上升时间和高低电平。测完 `kill <id>`，确认统计中 `err=0`。

**合格标准**：两种频率下 `i2cdetect` 均为 PASS；实测 SCL 频率与设置一致；后台循环 `err=0`。
**记录**：两种频率下的实测 SCL 频率和上升时间。

## 3. SD 卡

| 信号 | 引脚 |
|---|---|
| CLK | GPIO43 |
| CMD | GPIO44 |
| D0–D3 | GPIO39–GPIO42 |
| 电源使能 | GPIO45 |

### 20 MHz 读写

```
waveshare> mount -t sd -o freq=20 /sdcard
Name: SD32G ... Speed: ...
requested 20 MHz, negotiated 20000 kHz
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
33554432 bytes, 2.1 s, 15.2 MB/s
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
33554432 bytes, 1.6 s, 20.0 MB/s  verify ok
waveshare> umount /sdcard
```

### 40 MHz 读写

```
waveshare> mount -t sd -o freq=40 /sdcard
requested 40 MHz, negotiated 40000 kHz
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
```

### 抓波形

在已挂载的状态下启动后台读：

```
waveshare> watch dd if=/sdcard/t.bin of=/dev/null bs=64k
```

探 GPIO43（CLK）、GPIO44（CMD）、GPIO39（D0），测量 CLK 频率、幅度和过冲。测完 `kill <id>`，再 `umount /sdcard`。

另一个频率的测法相同：先 `umount /sdcard`，再用对应的 `freq=` 重新挂载。

**合格标准**：两种频率均挂载成功，且 `negotiated` 等于请求值；读回输出 `verify ok`；后台循环 `err=0`。
**记录**：两种频率下的写 / 读速度（MB/s）和实测 CLK 频率。

## 4. USB Host

插入 U 盘后执行：

```
waveshare> lsusb
usb host started, waiting for devices...
1 device(s) enumerated
MSC device at address 1 (not mounted; use: mount -t usb /usb)
waveshare> mount -t usb /usb
VID:PID 0781:5567  29xxx MB (... sectors x 512 bytes)
mounted at /usb
waveshare> dd if=/dev/urandom of=/usb/t.bin bs=64k count=1024
waveshare> dd if=/usb/t.bin of=/dev/null bs=64k
67108864 bytes, ... MB/s  verify ok
```

### 抓波形

```
waveshare> watch dd if=/usb/t.bin of=/dev/null bs=64k
```

在 USB 连接器上探 D+ / D−（最好用差分探头）。High-Speed 设备的信号幅度约 ±400 mV，Full-Speed 约 3.3 V。测完 `kill <id>`，再 `umount /usb`。

**合格标准**：枚举和挂载均成功；读回输出 `verify ok`；后台循环 `err=0`。
**记录**：VID:PID、写 / 读速度（MB/s）、总线速度（HS / FS）。

## 5. Wi-Fi

Wi-Fi 由 ESP32-C5 协处理器提供，通过 SDIO 与 P4 相连。

| 信号 | 引脚 |
|---|---|
| SDIO CLK | GPIO18 |
| SDIO CMD | GPIO19 |
| SDIO D0–D3 | GPIO14–GPIO17 |
| C5 复位 | GPIO54 |

### 连接

```
waveshare> iwconfig wifi0 essid <SSID> key <密码>
waveshare> ifup wifi0
wifi0: starting ESP-Hosted link to ESP32-C5 (SDIO slot 1)...
wifi0: coprocessor fw x.y.z
wifi0: connecting to '<SSID>' (20 s)...
wifi0: associated
wifi0: ip 192.168.1.51 mask 255.255.255.0 gw 192.168.1.1
wifi0: rssi -45 ch 36
waveshare> ifconfig
```

如果 `ifup wifi0` 报 `esp_hosted_init failed` 或 `esp_hosted_connect_to_slave failed`，先确认 C5 已烧录协处理器固件。

### 吞吐

| 方向 | 板端命令 | PC 端命令 |
|---|---|---|
| 上行（板 → PC） | `iperf -c <PC_IP> -t 30` | `iperf -s` |
| 下行（PC → 板） | `iperf -s` | `iperf -c <板子 IP> -t 30` |

测完用 `iperf -a` 停止 iperf，用 `ifdown wifi0` 关闭 Wi-Fi。

吞吐测试期间可探 GPIO18，SDIO CLK 应为 40 MHz。

**合格标准**：能读到协处理器固件版本；获取到 IP；RSSI 合理（近距离应高于 −60 dBm）；吞吐满足项目要求。
**记录**：协处理器版本、RSSI、信道、TCP 上行 / 下行速率（Mbps）。

<a id="display-backlight-touch"></a>

## 6. 屏幕 / 背光 / 触摸

### 显示

```
waveshare> fbtest colorbar     # 从左到右：白 黄 青 绿 品红 红 蓝 黑
waveshare> fbtest red
waveshare> fbtest green
waveshare> fbtest blue
waveshare> fbtest white        # 检查坏点和亮度均匀性
waveshare> fbtest black        # 检查漏光
waveshare> fbtest checker      # 32 px 黑白棋盘，检查边缘清晰度
```

### 背光

```
waveshare> backlight 10
waveshare> backlight 50
waveshare> backlight 100
```

### 触摸

`evtest` 会把屏幕变成画板：先清成黑色，并在四角和中心画出 5 个红色十字靶标。

```
waveshare> evtest
[4] started: evtest
draw on the panel and touch the 5 red targets; stop with: kill <id>
touch[0] down x=41 y=38
evtest: target 1/5 hit at x=41 y=38
touch[0] up
...
evtest: all targets hit, lcd.touch PASS (keep drawing or kill <id>)
waveshare> kill <id>
```

操作步骤：

1. 依次点击 5 个红色靶标，点中的靶标会变成绿色。5 个都变绿后，报告中的 `lcd.touch` 自动记为 PASS。
2. 用手指在屏幕上划线，笔迹应紧跟手指、连续无断点。在屏幕四边各划一条线，检查边缘区域也能响应。
3. 多根手指同时划动，每根手指的笔迹颜色不同（黄、青、品红、橙、白），可用来检查多点触摸。
4. 需要清屏时，`kill <id>` 后重新运行 `evtest`。

**合格标准**：彩条顺序正确；无坏点、无漏光；背光三档亮度变化明显；5 个靶标全部变绿；笔迹位置与手指一致，连续无断点、无跳点；多点触摸正常。
**记录**：屏幕型号、坏点数、触摸是否正常。

## 7. 摄像头

```
waveshare> v4l2-ctl --stream
[3] started: v4l2-ctl
[3] v4l2-ctl: iter=... err=0 ... MB/s
waveshare> v4l2-ctl --stop
```

预览期间每 5 秒打印一行统计，`iter` 为累计帧数。

**合格标准**：屏幕显示实时画面，色彩和方向正常，无撕裂；`err=0`。
**记录**：帧率（相邻两行统计的 `iter` 差值 ÷ 5）。

## 8. 音频

| 信号 | 引脚 | 方向 |
|---|---|---|
| SCLK | GPIO12 | P4 → codec |
| WS | GPIO10 | P4 → codec |
| DOUT | GPIO9 | P4 → codec |
| DIN | GPIO11 | codec → P4 |
| 功放使能 | GPIO53 | P4 → 功放 |

ES8389 默认不接 MCLK（R101 不焊，R104 将 MCLK 接到 DSDIN），编解码器时钟由 SCLK 派生，因此无需测量 GPIO13。

### 播放与时钟测量

```
waveshare> speaker-test                  # 1 kHz 正弦波，48 kHz 采样率
1000 Hz sine playing; stop with kill <id>. Probe GPIO12 SCLK / GPIO10 WS
```

播放期间测量：

| 信号 | 期望频率 |
|---|---|
| GPIO12 SCLK | 1.536 MHz |
| GPIO10 WS | 48 kHz |

用示波器或耳听确认喇叭输出为无失真的 1 kHz 音。可选操作：

```
waveshare> amixer set Master 30          # 播放音量 0–100 %，播放中即时生效
waveshare> amixer set Master 90
waveshare> kill all
waveshare> speaker-test -f 440           # 指定其他频率
waveshare> kill all
```

### 录音与回放

```
waveshare> mount -t sd /sdcard
waveshare> arecord -d 3 /sdcard/rec.wav  # 对着麦克风说话 3 秒；结束时打印两个麦克风的峰值 mic peak: FL=... FR=...
waveshare> aplay /sdcard/rec.wav         # 应回放刚才录下的声音
waveshare> aplay /sdcard/song.mp3        # 可选：播放 SD 卡中的 mp3 / aac / m4a / flac
```

### 麦克风直通

```
waveshare> alsaloop                      # 左麦克风直通左喇叭、右麦克风直通右喇叭；分别对着两个麦克风说话，对应一侧的喇叭应立即发声
waveshare> amixer set Capture 45         # 麦克风增益 0–60 dB，直通中即时生效
waveshare> amixer                        # 查看当前音量和增益
waveshare> kill <id>
```

**合格标准**：两路时钟频率正确；1 kHz 音清晰无杂音；录音回放清晰，且 `mic peak` 中 FL、FR 两路说话时都明显大于 0（某一路接近 0 说明该麦克风异常）；直通无明显延迟。
**记录**：SCLK / WS 实测频率、喇叭输出峰峰值（可选）。

## 9. 扩展排针 GPIO

可测引脚共 13 个，分布在 P1、P2 两个连接器上：

| 连接器 | 引脚号 → GPIO |
|---|---|
| P2 | 4 → GPIO4，5 → GPIO5，6 → GPIO24，7 → GPIO25，8 → GPIO36，9 → GPIO46，10 → GPIO47，11 → GPIO48，12 → GPIO51 |
| P1 | 6 → GPIO32，7 → GPIO33，8 → GPIO49，9 → GPIO50 |

注意：GPIO24 / GPIO25 同时是 USB 1.1 FS PHY 的 D− / D+（原理图网络名 USB1P1_N / USB1P1_P）；GPIO36 是 strapping 引脚，测试时不要外接会改变上电电平的负载。`gpioinfo` 会列出每个 GPIO 对应的连接器引脚。

### 批量测试

```
waveshare> gpioinfo
waveshare> gpioset all=1        # 全部置高，万用表逐脚测得 3.3 V
waveshare> gpioset all=0        # 全部置低，逐脚测得 0 V
waveshare> gpioset --blink all  # 1 Hz 方波，示波器逐脚观察
waveshare> kill <id>
waveshare> gpioget all          # 全部切为上拉输入并读取
```

### 单脚测试

每个引脚都映射为文件 `/gpio<N>/value` 和 `/gpio<N>/direction`，可用 `ls` / `cat` / `echo` 操作：

```
waveshare> ls /                         # gpio4 gpio5 ... gpio51
waveshare> echo 1 > /gpio4/value        # 置高，万用表测得 3.3 V
waveshare> echo 0 > /gpio4/value        # 置低，测得 0 V
waveshare> cat /gpio4/value             # 读取当前电平
waveshare> echo in > /gpio4/direction   # 切回上拉输入：悬空读到 1，接 GND 读到 0
waveshare> cat /gpio4/direction         # 输出 in 或 out
```

**合格标准**：每个引脚的置高、置低、翻转和输入读取均正确。
**记录**：异常引脚编号。

## 10. 用户 LED

| LED | ESP32-P4 | 点亮电平 |
|---|---|---|
| LED1 | GPIO2 | 低电平 |
| LED2 | GPIO3 | 低电平 |

```
waveshare> led all on            # 两个 LED 都亮
waveshare> led all off           # 两个 LED 都灭
waveshare> led 1 on              # 只亮 LED1，确认位置与丝印对应
waveshare> led 2 on              # 再亮 LED2
waveshare> led                   # 查看当前状态
LED1 (GPIO2): on
LED2 (GPIO3): on
waveshare> led blink             # LED1 / LED2 以 1 Hz 交替闪烁
waveshare> kill <id>             # 停止闪烁，两个 LED 熄灭
```

**合格标准**：两个 LED 都能单独点亮和熄灭，亮度正常，位置与丝印一致；交替闪烁正常。
**记录**：异常的 LED。

## 11. RS232

| 信号 | P1 引脚 | ESP32-P4 |
|---|---|---|
| RS232_T | 12 | UART2 TX，GPIO28 |
| RS232_R | 13 | UART2 RX，GPIO29 |
| GND | 14 | — |

默认 115200 8N1。所有 `rs232` 命令都可以加 `-b <波特率>`，只对当前这条命令有效。

### 与电脑互发

接 USB 转 RS232 模块：模块 TX 接 P1-13，RX 接 P1-12，GND 接 P1-14；电脑串口工具设为 115200 8N1。

```
waveshare> rs232 send hello            # 电脑上应收到 hello
waveshare> rs232 echo                  # 后台回显：电脑发什么，板子打印并原样发回
rs232 12 bytes: hello board!
rs232 echoed 12/12 bytes
waveshare> rs232 send hi             # echo 运行时也可以用 send 主动发送
waveshare> kill <id>
```

**合格标准**：电脑能收到 `rs232 send` 的内容；电脑发送的数据板子能打印出来，且电脑收到的回显与发送内容一致。
**记录**：测试波特率。

## 12. RS485

| 信号 | P1 引脚 | ESP32-P4 |
|---|---|---|
| A | 10（A1） | UART1 TX GPIO26 / RX GPIO27，经收发芯片 |
| B | 11（B1） | 同上 |
| GND | 14 | — |
| 方向控制 DE/RE | — | GPIO34，由 UART 自动控制 |

默认 115200 8N1，半双工。所有 `rs485` 命令都可以加 `-b <波特率>`。

RS485 是半双工，不能自己回环。用 USB 转 RS485 模块接电脑：模块 A 接 P1-10，B 接 P1-11，GND 接 P1-14；电脑串口工具设为 115200 8N1。

```
waveshare> rs485 send hello            # 电脑上应收到 hello
waveshare> rs485 echo                  # 后台回显：电脑发什么，板子打印并原样发回
[2] started: rs485 echo
rs485 12 bytes: hello board!
rs485 echoed 12/12 bytes
waveshare> rs485 send hi             # echo 运行时也可以用 send 主动发送
waveshare> kill <id>
```

板子收到数据后，`rs485.rx` 自动记为 PASS。电脑上应看到发出的内容被原样回显。收不到数据时，先检查 A、B 是否接反。

**合格标准**：电脑能收到 `rs485 send` 的内容；电脑发送的数据板子能打印出来，且电脑收到的回显与发送内容一致。
**记录**：测试波特率。

## 13. CAN

| 信号 | P1 引脚 | ESP32-P4 |
|---|---|---|
| CANH | 15 | TWAI0 TX GPIO22 / RX GPIO21，经收发芯片 |
| CANL | 16 | 同上 |
| GND | 14 | — |

命令格式与 `examples/esp-idf/twai_network` 相同，控制器名固定为 `twai0`。默认 500 kbit/s，用 `twai_init twai0 -b <bps>` 修改，直到 `twai_deinit` 前一直有效。

### 自检

不需要外部设备，验证 P4 → CAN 收发芯片 → P4 这一段：

```
waveshare> twai_init twai0 --self-test --loopback   # self-test：发送不需要 ACK；loopback：接收自己发的帧
twai0 up: 500000 bps, TX GPIO22, RX GPIO21, loopback, self-test
waveshare> twai_dump twai0                          # 后台打印收到的帧
waveshare> twai_send twai0 123#DEADBEEF
sent 123#DEADBEEF
  twai0  123  [4]  DE AD BE EF                      # 收到自己发出的帧
waveshare> twai_deinit twai0
```

### 与 USB-CAN 分析仪互发

分析仪 CAN_H 接 P1-15，CAN_L 接 P1-16，总线两端各需要一个 120 Ω 终端电阻；分析仪设为 500 kbit/s。

```
waveshare> twai_init twai0
waveshare> twai_dump twai0                          # 分析仪发送的帧会打印出来
  twai0  321  [2]  11 22
waveshare> twai_send twai0 123#0102030405060708     # 分析仪上应收到该帧
waveshare> twai_info twai0                          # 状态应为 error-active，TEC / REC 为 0
waveshare> twai_dump twai0 --stop
waveshare> twai_deinit twai0
```

帧格式与 Linux `cansend` 相同：

| 写法 | 含义 |
|---|---|
| `123#DEADBEEF` | 标准帧，ID 0x123，4 字节数据 |
| `12345678#0011` | 扩展帧（ID 超过 3 位十六进制） |
| `123#R` | 远程帧 |

正常模式下没有其他节点应答时，`twai_send` 会报 `send failed`（无 ACK）。总线进入 bus-off 后，用 `twai_recover twai0` 恢复。

**合格标准**：自检能收到自己发出的帧；与分析仪双向收发正常；`twai_info` 中 TEC / REC 为 0。
**记录**：测试速率、异常现象。

## 14. 汇总报告

`test all` 依次自动运行：系统信息、NVS、I2C 100k / 400k 扫描、SD 卡 20 / 40 MHz 读写校验、U 盘读写校验。未插 SD 卡或 U 盘时，对应项记为 SKIP。

```
waveshare> test all
waveshare> test report
ITEM                           RESULT  VALUE                  NOTE
sys.uname                      PASS
nvs.rw                         PASS    count=3
i2c.detect.100k                PASS                           es8389@0x10:y gt911:y
i2c.detect.400k                PASS                           es8389@0x10:y gt911:y
sd.mount.20M                   PASS    20000 kHz              SD32G
sd.write.20M                   PASS    15.20 MB/s
sd.read.20M                    PASS    20.01 MB/s
sd.mount.40M                   PASS    40000 kHz              SD32G
...
usb.msc.mount                  PASS    0781:5567
...
pass=<自动测试通过项数> fail=0 skip=0 manual=0
```

说明：

- `test all` 会先清空之前的记录，因此要在人工项之前运行。
- Wi-Fi 运行 `ifup wifi0` 后会记为 `wifi.link`。
- 屏幕、摄像头、音频、GPIO 等人工项运行后会以 MANUAL 出现在报告中，需要测试人员填写结论。
- `test clear` 可手动清空报告。

## 附录：示波器探测点

| 项目 | 探测点 | 期望 | 启动命令 |
|---|---|---|---|
| I2C | GPIO8 SCL / GPIO7 SDA | 100 kHz 或 400 kHz | `watch i2cget -c 0x10 -r 0x00` |
| SD 卡 | GPIO43 CLK / GPIO44 CMD / GPIO39 D0 | 20 MHz 或 40 MHz | `watch dd if=/sdcard/t.bin of=/dev/null bs=64k` |
| USB | 连接器 D+ / D− | HS ±400 mV / FS 3.3 V | `watch dd if=/usb/t.bin of=/dev/null bs=64k` |
| SDIO（C5） | GPIO18 CLK / GPIO19 CMD | 40 MHz | iperf 吞吐测试期间 |
| 音频 | GPIO12 SCLK / GPIO10 WS | 1.536 MHz / 48 kHz | `speaker-test` |
| 排针 | 各引脚 | 3.3 V / 0 V / 1 Hz 方波 | `gpioset all=1` / `all=0` / `--blink all` |
