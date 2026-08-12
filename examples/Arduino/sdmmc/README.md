# sdmmc

## 作用

使用 4-bit SDMMC 接口初始化板载 microSD 卡，打印卡片和文件系统容量，并对测试文件进行读写。

## 使用方法

1. 将 microSD 卡插入开发板。
2. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C`。
3. 打开 `sdmmc.ino`，编译并上传。
4. 打开串口监视器，波特率设置为 `115200`。
5. 查看卡片信息以及 `/arduino_test.txt` 的写入和读取结果。
