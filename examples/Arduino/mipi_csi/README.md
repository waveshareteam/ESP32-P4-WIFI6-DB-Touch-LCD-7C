# mipi_csi

## 作用

初始化 MIPI-CSI 摄像头和 MIPI-DSI LCD，将摄像头采集到的实时图像显示在 LCD 上。当前示例使用 OV5647 摄像头和 1024×600 EK79007 LCD 配置。

## 使用方法

1. 将 OV5647 摄像头连接到开发板的 MIPI-CSI 接口。
2. 将 LCD 连接到开发板的 MIPI-DSI 接口。
3. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C`。
4. 打开 `mipi_csi.ino`，编译并上传。
5. 打开串口监视器，波特率设置为 `115200`。
6. 初始化成功后，LCD 会显示摄像头实时画面。
