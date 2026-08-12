# twai

## 作用

使用开发板上的 TWAI/CAN 收发器，以 500 kbit/s、接收监听模式接收总线报文，并打印报文 ID、DLC 和数据内容。

示例只接收报文，不向总线发送数据，并接受所有 CAN ID。

## 使用方法

1. 将开发板的 TWAI/CAN 接口连接到一个工作中的 CAN 总线，并确保总线速率为 `500 kbit/s`。
2. 在 Arduino IDE 中选择 `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C`。
3. 打开 `twai.ino`，编译并上传。
4. 打开串口监视器，波特率设置为 `115200`。
5. 当总线上出现报文时，串口会打印对应内容。
