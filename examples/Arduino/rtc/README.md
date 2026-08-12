# rtc

## 作用

通过 I2C 读取板载 PCF85063A 实时时钟，并每秒打印当前日期和时间。RTC 使用 SDA GPIO7、SCL GPIO8，I2C 地址为 `0x51`。

当 RTC 时间无效或振荡器停止时，示例会使用固件编译时间初始化 RTC；有效的 RTC 时间会在重启后保留。

## 使用方法

1. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C`。
2. 打开 `rtc.ino`，编译并上传。
3. 打开串口监视器，波特率设置为 `115200`。
4. 查看每秒输出的 RTC 时间。

如果需要强制使用本次固件的编译时间覆盖 RTC，将 `rtc_set_time_on_boot` 改为 `true` 后重新上传；使用一次后建议恢复为 `false`。
