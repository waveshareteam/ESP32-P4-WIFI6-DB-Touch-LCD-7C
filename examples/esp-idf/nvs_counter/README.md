# NVS Counter

[中文版本](./README_ZH.md)

## What It Does

Beginner-friendly example that stores a boot counter in NVS flash.

NVS is the usual place to save small settings such as calibration values,
device names, user preferences, or provisioning state.

## Hardware Required

- One ESP32-P4 board.
- USB cable.

No external peripheral is required.

## How to Run

```bash
cd examples/esp-idf/03_nvs_counter
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## Expected Result

The serial monitor prints a saved boot count:

```text
Saved boot count: 1
Press reset. The number should increase after each boot.
```

Press reset and the count should increase.

## Troubleshooting

Erase flash before flashing:

```bash
idf.py -p PORT erase-flash flash monitor
```

### What to Reuse

- `nvs_flash_init()` initialization and recovery.
- `nvs_open()`, `nvs_get_u32()`, `nvs_set_u32()`, and `nvs_commit()` flow.
- Pattern for saving simple settings persistently.
