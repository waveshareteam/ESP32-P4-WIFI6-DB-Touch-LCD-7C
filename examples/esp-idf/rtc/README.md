# PCF85063A RTC Function Test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

This example tests the Waveshare `pcf85063a` component with a PCF85063A RTC.

The example performs these tests in order:

1. Initialize I2C and the PCF85063A.
2. Read the current time; initialize it from the build time only if it is clearly invalid.
3. Test separate time write/read.
4. Test separate date write/read.
5. Test combined time/date write/read and restore the original time.
6. Test Offset and RAM register read/write and restore the original RAM value.
7. Set and read an Alarm, then wait for the Alarm Flag and GPIO20 interrupt.
8. Start a 5-second, 1 Hz Timer and test countdown, Timer Flag, flag clearing, and GPIO20 interrupt.
9. Perform a software reset and verify that the RTC can still be read afterward.
10. After all tests finish, print the current RTC time once per second.

The component does not provide dedicated Offset or RAM helper functions. These registers are tested through its public generic register read/write APIs.

## Hardware Required

| Signal | ESP32-P4 |
|---|---:|
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| RTC INT | GPIO20 |
| I2C address | `0x51` |

The PCF85063A `INT` output is open-drain and active-low. The example configures GPIO20 as an input with pull-up and falling-edge interrupt. An external pull-up resistor is recommended for the hardware.

To skip the GPIO interrupt test, set `RTC_TEST_ENABLE_INTERRUPT` to `0` near the top of `main/main.c`. The Alarm and Timer flag tests still run, while the GPIO20 ISR and interrupt assertions are omitted.

## How to Run

```powershell
cd examples/esp-idf/rtc
python -m pip install --upgrade esp-bmgr-assist
idf.py bmgr -b esp32_p4_wifi6_db_touch_lcd_7c
idf.py set-target esp32p4
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the actual serial port.

## Expected Result

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

The final `RTC time` line repeats once per second. Its date and time depend on
the retained RTC value or the build timestamp used during initialization.

## Troubleshooting

If I2C initialization or device access fails, check SDA/SCL, power, the device address, and I2C pull-ups. If a Flag is set but no GPIO20 interrupt is received, check the GPIO20 connection and the INT pull-up.
