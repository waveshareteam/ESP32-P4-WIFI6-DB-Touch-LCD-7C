# GPIO Console Example

This example uses the Arduino-ESP32 `Console` library to configure and test GPIOs interactively through the serial monitor. It is based on the Arduino core `ConsoleGPIO` example.

## Requirements

- Select `Waveshare ESP32-P4-WIFI6-DB-Touch-LCD-7C` in Arduino IDE.
- Use the USB serial port and set the serial monitor to `115200` baud.
- Ensure the Arduino-ESP32 `Console` library is available. It is included with the Arduino core installation.

## Usage

After uploading, open the serial monitor, set line ending to `Newline`, and enter `help` to list commands.

```text
gpio read <pin>
gpio write <pin> <0|1>
gpio mode <pin> <in|out|in_pu|in_pd>
led <on|off>
```

Examples:

```text
gpio mode 2 out
gpio write 2 0
gpio write 2 1
gpio mode 4 in_pu
gpio read 4
led on
led off
```

## Board Notes

- `LED_BUILTIN` is GPIO2 and is active-low on this board. `led on` and `led off` account for this polarity.
- GPIO3 is the second user LED.
- Do not use GPIOs reserved by onboard peripherals while their corresponding examples or features are active. This includes the MIPI display, touch controller, camera, SDMMC, ESP-Hosted SDIO, audio codec, and RS485/RS232 transceivers.
- Use `gpio mode` before `gpio write`. Writing a pin configured as an input is not a valid output test.

## Expected Output

```text
gpio> gpio mode 2 out
GPIO 2 mode set to out
gpio> led on
GPIO 2 set to 0 (LOW)
gpio> gpio read 2
GPIO 2 = 0 (LOW)
```
