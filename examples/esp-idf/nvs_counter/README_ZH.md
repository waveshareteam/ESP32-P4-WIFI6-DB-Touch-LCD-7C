# NVS 计数器

[English Version](./README.md)

## 功能

这是一个适合初学者的示例，用 NVS flash 保存启动计数。

NVS 通常用于保存少量设置，例如校准值、设备名称、用户偏好或配网状态。

## 硬件需求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 怎么运行

```bash
cd examples/esp-idf/03_nvs_counter
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## 预期现象

串口监视器会打印已保存的启动次数：

```text
Saved boot count: 1
Press reset. The number should increase after each boot.
```

按下复位键后，计数应递增。

## 常见问题

烧录前擦除 flash：

```bash
idf.py -p PORT erase-flash flash monitor
```

### 可复用内容

- `nvs_flash_init()` 初始化和恢复流程。
- `nvs_open()`、`nvs_get_u32()`、`nvs_set_u32()` 和 `nvs_commit()` 流程。
- 持久保存简单设置的模式。
