# PCF85063A RTC 功能测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

本示例用于测试 Waveshare `pcf85063a` 组件和 PCF85063A RTC 芯片。

程序启动后依次执行：

1. 初始化 I2C 和 PCF85063A。
2. 读取当前时间；如果时间明显无效，仅使用编译时间初始化一次。
3. 测试时间单独读写。
4. 测试日期单独读写。
5. 测试时间和日期组合读写，并恢复原时间。
6. 测试 Offset 和 RAM 寄存器读写，并恢复 RAM 原值。
7. 设置 Alarm，读取 Alarm 配置，等待 Alarm Flag 和 GPIO20 中断。
8. 启动 5 秒、1 Hz Timer，测试倒计时、Timer Flag、清除 Flag 和 GPIO20 中断。
9. 执行软件复位，并验证复位后仍可读取 RTC。
10. 所有测试完成后，每秒输出一次 RTC 当前时间。

Offset 和 RAM 没有独立的组件 API，示例通过组件公开的通用寄存器读写接口测试。

## 硬件需求

| 信号 | ESP32-P4 |
|---|---:|
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| RTC INT | GPIO20 |
| I2C 地址 | `0x51` |

PCF85063A 的 `INT` 为开漏、低有效输出。示例将 GPIO20 配置为输入、上拉和下降沿中断；硬件上建议使用外部上拉电阻。

如不需要测试 GPIO 中断，可将 `main/main.c` 顶部的 `RTC_TEST_ENABLE_INTERRUPT` 设置为 `0`。此时会跳过 GPIO20 ISR 和中断断言，但仍会测试 Alarm/Timer 标志位。

## 怎么运行

```powershell
cd examples/esp-idf/rtc
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4
idf.py build
idf.py -p COMx flash monitor
```

将 `COMx` 替换为实际串口号。

## 预期现象

```text
I (...) rtc_test: RTC time is valid; preserving it
I (...) rtc_test: PASS: offset register read/write
I (...) rtc_test: PASS: RAM register read/write
I (...) rtc_test: PASS: set/get time
I (...) rtc_test: PASS: set/get date
I (...) rtc_test: PASS: set/get alarm
I (...) rtc_test: PASS: alarm flag trigger
I (...) rtc_test: PASS: alarm GPIO20 interrupt
I (...) rtc_test: PASS: timer countdown
I (...) rtc_test: PASS: timer flag trigger
I (...) rtc_test: PASS: timer GPIO20 interrupt
I (...) rtc_test: PASS: RTC readable after reset
I (...) rtc_test: All exposed PCF85063A function tests finished
I (...) rtc_test: Starting 1-second RTC time output
I (...) rtc_test: RTC time:  2026.7.29  3 14:30:0
```

最后一行 `RTC time` 会每秒重复输出；实际日期和时间取决于 RTC 中保留的值，或初始化时写入的固件编译时间。

## 常见问题

如果 I2C 初始化或芯片访问失败，优先检查 SDA/SCL、芯片供电、地址和 I2C 上拉；如果 Flag 置位但没有 GPIO20 中断，检查 GPIO20 连线和 INT 上拉。
