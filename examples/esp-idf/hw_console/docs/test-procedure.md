# ESP32-P4-WIFI6-DB-Touch-LCD-7C Hardware Verification Procedure

[English](test-procedure.md) | [简体中文](test-procedure_ZH.md)

This procedure accompanies the `examples/esp-idf/hw_console` firmware. It verifies the on-board peripherals one by one and captures key signal waveforms.

## Conventions

- Serial settings: 115200 8N1, prompt `waveshare>`. Type `help` at any time to list all commands.
- In the examples, text after `waveshare>` is the command to enter; the other lines are the expected output. Exact values vary from board to board.
- The **Pass criteria** at the end of each section decide the result; **Record** lists the measurements and observations to write down.
- Background jobs: list them with `jobs` and stop them with `kill <id>` or `kill all`. A background job prints one statistics line (`iter` / `err` / `MB/s`) every 5 seconds.
- Capturing waveforms: start a background loop with `watch <command>`, attach the probe, take the measurement, then `kill` the job. All probe points are collected in the [appendix](#appendix-oscilloscope-probe-points).

## Test Overview

| Section | Item | Method | Equipment |
|---|---|---|---|
| [1](#1-system-information-and-nvs) | System information, NVS | Automatic | — |
| [2](#2-i2c) | I2C 100 kHz / 400 kHz | Automatic + waveform | Oscilloscope |
| [3](#3-sd-card) | SD card 20 MHz / 40 MHz | Automatic + waveform | microSD card, oscilloscope |
| [4](#4-usb-host) | USB Host | Automatic + waveform | USB drive, oscilloscope |
| [5](#5-wi-fi) | Wi-Fi (ESP32-C5) | Semi-automatic | AP, PC (iperf 2) |
| [6](#6-display-backlight-and-touch) | Display / backlight / touch | Manual | Display |
| [7](#7-camera) | Camera | Manual | Display, camera |
| [8](#8-audio) | Audio | Manual + waveform | Speaker, oscilloscope, microSD card |
| [9](#9-expansion-header-gpio) | Expansion header GPIO | Manual | Multimeter, oscilloscope |
| [10](#10-user-leds) | User LEDs | Manual | — |
| [11](#11-rs232) | RS232 | Manual | USB-to-RS232 module, PC |
| [12](#12-rs485) | RS485 | Manual | USB-to-RS485 module, PC |
| [13](#13-can) | CAN | Semi-automatic | USB-CAN analyzer, 120 Ω terminator |
| [14](#14-summary-report) | Summary report | Automatic | — |

Recommended order: run `test all` first (see section 14) to complete all automatic items, then do the waveform measurements and manual items in sections 2–13, and finally view the summary with `test report`.

## 0. Preparation

### Equipment

| Item | Requirement |
|---|---|
| Board under test | Flashed with the hw_console firmware; the Wi-Fi test also needs the ESP32-C5 flashed with `examples/esp-idf/wifi_iperf/cp` |
| microSD card | High Speed capable (Class 10 or UHS-I), FAT32; data on the card will be overwritten |
| USB drive | FAT32, any capacity, operating current ≤ 500 mA |
| Wi-Fi AP | 2.4 GHz or 5 GHz, WPA2; preferably connect the PC to the same router by cable |
| PC | Serial terminal; iperf **2.x** (check with `iperf --version`; iperf3 does not work) |
| Oscilloscope / logic analyzer | Bandwidth ≥ 100 MHz (SD CLK is 40 MHz) |
| Multimeter | For header levels |
| Display | Waveshare EK79007 1024×600 MIPI-DSI display with GT911 touch |
| Camera | OV5647 MIPI-CSI module |
| Speaker | Connected to the on-board amplifier output |
| Audio file (optional) | An mp3 file on the SD card for `aplay` |
| USB-to-RS232 module | Connected to the PC, for RS232 transmit and receive tests |
| USB-to-RS485 module | Connected to the PC, for RS485 transmit and receive tests |
| USB-CAN analyzer | 500 kbit/s, with a 120 Ω terminator |

### Power-on check

After power-up the serial port should print:

```
ESP32-P4-WIFI6-DB-Touch-LCD-7C factory test console
Type 'help' for the command list. Nothing is initialised until you use it.
waveshare>
```

The firmware does not initialize any peripheral at boot; each peripheral is initialized the first time a related command is used.

## 1. System Information and NVS

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

**Pass criteria**: Flash is 32 MB, the PSRAM size is correct, and `nvs test` prints `nvs ok`.
**Record**: Chip revision and MAC address.

## 2. I2C

| Signal | Pin |
|---|---|
| SDA | GPIO7 |
| SCL | GPIO8 |

Devices on the bus:

| Device | 7-bit address | Note |
|---|---|---|
| ES8389 audio codec | `0x10` | Configured as `0x20` in Board Manager; the codec driver shifts it right by one bit |
| GT911 touch | `0x5d` or `0x14` | Detected only when the display is connected |

The backlight is driven by an LEDC output on GPIO30, not by a device on the I2C bus.

### Scan

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

Optional register spot checks: `i2cget -c 0x10 -r 0x00`, `i2cdump -c 0x10`.

### Capture the waveform

```
waveshare> watch i2cget -c 0x10 -r 0x00
```

Probe GPIO8 (SCL) and GPIO7 (SDA). At both 100 kHz and 400 kHz, measure the SCL period (it should be 10 µs / 2.5 µs), the rise time and the high and low levels. When done, `kill <id>` and confirm `err=0` in the statistics.

**Pass criteria**: `i2cdetect` is PASS at both frequencies; the measured SCL frequency matches the setting; the background loop shows `err=0`.
**Record**: Measured SCL frequency and rise time at both frequencies.

## 3. SD Card

| Signal | Pin |
|---|---|
| CLK | GPIO43 |
| CMD | GPIO44 |
| D0–D3 | GPIO39–GPIO42 |
| Power enable | GPIO45 |

### 20 MHz read and write

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

### 40 MHz read and write

```
waveshare> mount -t sd -o freq=40 /sdcard
requested 40 MHz, negotiated 40000 kHz
waveshare> dd if=/dev/urandom of=/sdcard/t.bin bs=64k count=512
waveshare> dd if=/sdcard/t.bin of=/dev/null bs=64k
```

### Capture the waveform

With the card mounted, start a background read:

```
waveshare> watch dd if=/sdcard/t.bin of=/dev/null bs=64k
```

Probe GPIO43 (CLK), GPIO44 (CMD) and GPIO39 (D0), and measure the CLK frequency, amplitude and overshoot. When done, `kill <id>`, then `umount /sdcard`.

The other frequency is measured the same way: `umount /sdcard`, then mount again with the matching `freq=`.

**Pass criteria**: Mounting succeeds at both frequencies and `negotiated` equals the requested value; the read-back prints `verify ok`; the background loop shows `err=0`.
**Record**: Write and read speeds (MB/s) and the measured CLK frequency at both frequencies.

## 4. USB Host

Insert a USB drive and run:

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

### Capture the waveform

```
waveshare> watch dd if=/usb/t.bin of=/dev/null bs=64k
```

Probe D+ / D− on the USB connector (a differential probe is best). A High-Speed device has a signal swing of about ±400 mV and a Full-Speed device about 3.3 V. When done, `kill <id>`, then `umount /usb`.

**Pass criteria**: Enumeration and mounting succeed; the read-back prints `verify ok`; the background loop shows `err=0`.
**Record**: VID:PID, write and read speeds (MB/s), and bus speed (HS / FS).

## 5. Wi-Fi

Wi-Fi is provided by the ESP32-C5 coprocessor, which is connected to the P4 over SDIO.

| Signal | Pin |
|---|---|
| SDIO CLK | GPIO18 |
| SDIO CMD | GPIO19 |
| SDIO D0–D3 | GPIO14–GPIO17 |
| C5 reset | GPIO54 |

### Connect

```
waveshare> iwconfig wifi0 essid <SSID> key <password>
waveshare> ifup wifi0
wifi0: starting ESP-Hosted link to ESP32-C5 (SDIO slot 1)...
wifi0: coprocessor fw x.y.z
wifi0: connecting to '<SSID>' (20 s)...
wifi0: associated
wifi0: ip 192.168.1.51 mask 255.255.255.0 gw 192.168.1.1
wifi0: rssi -45 ch 36
waveshare> ifconfig
```

If `ifup wifi0` reports `esp_hosted_init failed` or `esp_hosted_connect_to_slave failed`, first make sure the C5 has the coprocessor firmware flashed.

### Throughput

| Direction | Board command | PC command |
|---|---|---|
| Uplink (board → PC) | `iperf -c <PC_IP> -t 30` | `iperf -s` |
| Downlink (PC → board) | `iperf -s` | `iperf -c <board IP> -t 30` |

When done, stop iperf with `iperf -a` and turn Wi-Fi off with `ifdown wifi0`.

During the throughput test you can probe GPIO18; the SDIO CLK should be 40 MHz.

**Pass criteria**: The coprocessor firmware version can be read; an IP address is obtained; RSSI is reasonable (above −60 dBm at close range); throughput meets the project requirement.
**Record**: Coprocessor version, RSSI, channel, TCP uplink and downlink rates (Mbps).

## 6. Display, Backlight and Touch

### Display

```
waveshare> fbtest colorbar     # left to right: white yellow cyan green magenta red blue black
waveshare> fbtest red
waveshare> fbtest green
waveshare> fbtest blue
waveshare> fbtest white        # check dead pixels and brightness uniformity
waveshare> fbtest black        # check light leakage
waveshare> fbtest checker      # 32 px black-and-white checkerboard, check edge sharpness
```

### Backlight

```
waveshare> backlight 10
waveshare> backlight 50
waveshare> backlight 100
```

### Touch

`evtest` turns the screen into a drawing board: it clears the screen to black and draws five red cross targets at the four corners and the center.

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

Steps:

1. Tap the five red targets one by one; a target turns green when hit. Once all five are green, `lcd.touch` is recorded as PASS in the report.
2. Draw lines with a finger. The stroke should follow the finger closely and continuously without gaps. Draw a line along each edge of the screen to check that the edge areas respond.
3. Draw with several fingers at once; each finger uses a different color (yellow, cyan, magenta, orange, white), which lets you check multi-touch.
4. To clear the screen, `kill <id>` and run `evtest` again.

**Pass criteria**: The color bar order is correct; no dead pixels or light leakage; the three backlight levels differ clearly; all five targets turn green; strokes match the finger position without gaps or jumps; multi-touch works.
**Record**: Display model, number of dead pixels, whether touch works.

## 7. Camera

```
waveshare> v4l2-ctl --stream
[3] started: v4l2-ctl
[3] v4l2-ctl: iter=... err=0 ... MB/s
waveshare> v4l2-ctl --stop
```

During the preview, one statistics line is printed every 5 seconds; `iter` is the cumulative frame count.

**Pass criteria**: The display shows a live image with correct colors and orientation and no tearing; `err=0`.
**Record**: Frame rate (difference of `iter` between two consecutive lines ÷ 5).

## 8. Audio

| Signal | Pin | Direction |
|---|---|---|
| SCLK | GPIO12 | P4 → codec |
| WS | GPIO10 | P4 → codec |
| DOUT | GPIO9 | P4 → codec |
| DIN | GPIO11 | codec → P4 |
| Amplifier enable | GPIO53 | P4 → amplifier |

The ES8389 has no MCLK connected by default (R101 is not populated, and R104 connects MCLK to DSDIN). The codec clock is derived from SCLK, so there is no need to measure GPIO13.

### Playback and clock measurement

```
waveshare> speaker-test                  # 1 kHz sine wave, 48 kHz sample rate
1000 Hz sine playing; stop with kill <id>. Probe GPIO12 SCLK / GPIO10 WS
```

Measure during playback:

| Signal | Expected frequency |
|---|---|
| GPIO12 SCLK | 1.536 MHz |
| GPIO10 WS | 48 kHz |

Use the oscilloscope or your ears to confirm that the speaker outputs an undistorted 1 kHz tone. Optional commands:

```
waveshare> amixer set Master 30          # playback volume 0–100 %, takes effect immediately during playback
waveshare> amixer set Master 90
waveshare> kill all
waveshare> speaker-test -f 440           # use a different frequency
waveshare> kill all
```

### Record and play back

```
waveshare> mount -t sd /sdcard
waveshare> arecord -d 3 /sdcard/rec.wav  # speak into the microphone for 3 seconds; prints the peaks of both microphones at the end: mic peak: FL=... FR=...
waveshare> aplay /sdcard/rec.wav         # should play back what you just recorded
waveshare> aplay /sdcard/song.mp3        # optional: play an mp3 / aac / m4a / flac file from the SD card
```

### Microphone pass-through

```
waveshare> alsaloop                      # left microphone to left speaker, right microphone to right speaker; speak into each microphone and the matching speaker should sound immediately
waveshare> amixer set Capture 45         # microphone gain 0–60 dB, takes effect immediately during pass-through
waveshare> amixer                        # show current volume and gain
waveshare> kill <id>
```

**Pass criteria**: Both clock frequencies are correct; the 1 kHz tone is clear and free of noise; the recording plays back clearly, and both FL and FR in `mic peak` are clearly above 0 while speaking (a channel close to 0 means that microphone is faulty); the pass-through has no noticeable delay.
**Record**: Measured SCLK / WS frequencies, and optionally the speaker output peak-to-peak voltage.

## 9. Expansion Header GPIO

There are 13 testable pins on the P1 and P2 connectors:

| Connector | Pin number → GPIO |
|---|---|
| P2 | 4 → GPIO4, 5 → GPIO5, 6 → GPIO24, 7 → GPIO25, 8 → GPIO36, 9 → GPIO46, 10 → GPIO47, 11 → GPIO48, 12 → GPIO51 |
| P1 | 6 → GPIO32, 7 → GPIO33, 8 → GPIO49, 9 → GPIO50 |

Notes: GPIO24 / GPIO25 are also D− / D+ of the USB 1.1 FS PHY (schematic nets USB1P1_N / USB1P1_P). GPIO36 is a strapping pin; do not attach a load that changes the power-on level during the test. `gpioinfo` lists the connector pin of every GPIO.

### Batch test

```
waveshare> gpioinfo
waveshare> gpioset all=1        # all high, measure 3.3 V on each pin with the multimeter
waveshare> gpioset all=0        # all low, measure 0 V on each pin
waveshare> gpioset --blink all  # 1 Hz square wave, observe each pin with the oscilloscope
waveshare> kill <id>
waveshare> gpioget all          # switch all to pull-up inputs and read them
```

### Single-pin test

Every pin is mapped to the files `/gpio<N>/value` and `/gpio<N>/direction`, which can be handled with `ls` / `cat` / `echo`:

```
waveshare> ls /                         # gpio4 gpio5 ... gpio51
waveshare> echo 1 > /gpio4/value        # drive high, 3.3 V on the multimeter
waveshare> echo 0 > /gpio4/value        # drive low, 0 V
waveshare> cat /gpio4/value             # read the current level
waveshare> echo in > /gpio4/direction   # back to pull-up input: floating reads 1, tied to GND reads 0
waveshare> cat /gpio4/direction         # prints in or out
```

**Pass criteria**: Driving high, driving low, toggling and input reading are correct on every pin.
**Record**: Numbers of any faulty pins.

## 10. User LEDs

| LED | ESP32-P4 | On level |
|---|---|---|
| LED1 | GPIO2 | Low |
| LED2 | GPIO3 | Low |

```
waveshare> led all on            # both LEDs on
waveshare> led all off           # both LEDs off
waveshare> led 1 on              # only LED1, confirm its position matches the silkscreen
waveshare> led 2 on              # then LED2
waveshare> led                   # show the current state
LED1 (GPIO2): on
LED2 (GPIO3): on
waveshare> led blink             # LED1 / LED2 alternate at 1 Hz
waveshare> kill <id>             # stop blinking, both LEDs go off
```

**Pass criteria**: Both LEDs can be turned on and off individually with normal brightness, positions match the silkscreen, and alternate blinking works.
**Record**: Any faulty LED.

## 11. RS232

| Signal | P1 pin | ESP32-P4 |
|---|---|---|
| RS232_T | 12 | UART2 TX, GPIO28 |
| RS232_R | 13 | UART2 RX, GPIO29 |
| GND | 14 | — |

Default 115200 8N1. Every `rs232` command accepts `-b <baud rate>`, which applies only to that command.

### Exchange data with a PC

Connect a USB-to-RS232 module: module TX to P1-13, RX to P1-12, GND to P1-14; set the PC serial tool to 115200 8N1.

```
waveshare> rs232 send hello            # the PC should receive hello
waveshare> rs232 echo                  # background echo: whatever the PC sends, the board prints and sends back
rs232 12 bytes: hello board!
rs232 echoed 12/12 bytes
waveshare> rs232 send hi             # send can also be used while echo is running
waveshare> kill <id>
```

**Pass criteria**: The PC receives the content of `rs232 send`; data sent by the PC is printed by the board, and the echo received by the PC matches what was sent.
**Record**: Baud rate used.

## 12. RS485

| Signal | P1 pin | ESP32-P4 |
|---|---|---|
| A | 10 (A1) | UART1 TX GPIO26 / RX GPIO27, through the transceiver |
| B | 11 (B1) | Same as above |
| GND | 14 | — |
| Direction control DE/RE | — | GPIO34, controlled automatically by the UART |

Default 115200 8N1, half duplex. Every `rs485` command accepts `-b <baud rate>`.

RS485 is half duplex and cannot loop back on its own. Use a USB-to-RS485 module on the PC: module A to P1-10, B to P1-11, GND to P1-14; set the PC serial tool to 115200 8N1.

```
waveshare> rs485 send hello            # the PC should receive hello
waveshare> rs485 echo                  # background echo: whatever the PC sends, the board prints and sends back
[2] started: rs485 echo
rs485 12 bytes: hello board!
rs485 echoed 12/12 bytes
waveshare> rs485 send hi             # send can also be used while echo is running
waveshare> kill <id>
```

Once the board receives data, `rs485.rx` is recorded as PASS automatically. The PC should see what it sent echoed back unchanged. If nothing is received, first check whether A and B are swapped.

**Pass criteria**: The PC receives the content of `rs485 send`; data sent by the PC is printed by the board, and the echo received by the PC matches what was sent.
**Record**: Baud rate used.

## 13. CAN

| Signal | P1 pin | ESP32-P4 |
|---|---|---|
| CANH | 15 | TWAI0 TX GPIO22 / RX GPIO21, through the transceiver |
| CANL | 16 | Same as above |
| GND | 14 | — |

The command format is the same as in `examples/esp-idf/twai_network`, and the controller name is fixed as `twai0`. The default rate is 500 kbit/s; change it with `twai_init twai0 -b <bps>`, which stays in effect until `twai_deinit`.

### Self-test

No external device is needed; this verifies the path P4 → CAN transceiver → P4:

```
waveshare> twai_init twai0 --self-test --loopback   # self-test: transmitting needs no ACK; loopback: receive the frames you send
twai0 up: 500000 bps, TX GPIO22, RX GPIO21, loopback, self-test
waveshare> twai_dump twai0                          # print received frames in the background
waveshare> twai_send twai0 123#DEADBEEF
sent 123#DEADBEEF
  twai0  123  [4]  DE AD BE EF                      # the frame just sent is received
waveshare> twai_deinit twai0
```

### Exchange frames with a USB-CAN analyzer

Connect the analyzer CAN_H to P1-15 and CAN_L to P1-16. A 120 Ω terminator is needed at each end of the bus. Set the analyzer to 500 kbit/s.

```
waveshare> twai_init twai0
waveshare> twai_dump twai0                          # frames sent by the analyzer are printed
  twai0  321  [2]  11 22
waveshare> twai_send twai0 123#0102030405060708     # the analyzer should receive this frame
waveshare> twai_info twai0                          # state should be error-active, TEC / REC are 0
waveshare> twai_dump twai0 --stop
waveshare> twai_deinit twai0
```

The frame format is the same as Linux `cansend`:

| Notation | Meaning |
|---|---|
| `123#DEADBEEF` | Standard frame, ID 0x123, 4 data bytes |
| `12345678#0011` | Extended frame (ID longer than 3 hex digits) |
| `123#R` | Remote frame |

In normal mode, `twai_send` reports `send failed` (no ACK) when no other node acknowledges. After the bus goes bus-off, recover with `twai_recover twai0`.

**Pass criteria**: The self-test receives the frame it sent; transmit and receive with the analyzer work in both directions; TEC / REC in `twai_info` are 0.
**Record**: Rate used and any abnormal behavior.

## 14. Summary Report

`test all` automatically runs, in order: system information, NVS, I2C 100k / 400k scan, SD card 20 / 40 MHz read and write verification, and USB drive read and write verification. Items are recorded as SKIP when no SD card or USB drive is inserted.

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
pass=<number of passed automatic items> fail=0 skip=0 manual=0
```

Notes:

- `test all` clears earlier records first, so run it before the manual items.
- After `ifup wifi0`, Wi-Fi is recorded as `wifi.link`.
- Manual items such as display, camera, audio and GPIO appear in the report as MANUAL after they run; the tester fills in the conclusion.
- `test clear` clears the report manually.

## Appendix: Oscilloscope Probe Points

| Item | Probe point | Expected | Start command |
|---|---|---|---|
| I2C | GPIO8 SCL / GPIO7 SDA | 100 kHz or 400 kHz | `watch i2cget -c 0x10 -r 0x00` |
| SD card | GPIO43 CLK / GPIO44 CMD / GPIO39 D0 | 20 MHz or 40 MHz | `watch dd if=/sdcard/t.bin of=/dev/null bs=64k` |
| USB | Connector D+ / D− | HS ±400 mV / FS 3.3 V | `watch dd if=/usb/t.bin of=/dev/null bs=64k` |
| SDIO (C5) | GPIO18 CLK / GPIO19 CMD | 40 MHz | During the iperf throughput test |
| Audio | GPIO12 SCLK / GPIO10 WS | 1.536 MHz / 48 kHz | `speaker-test` |
| Header | Each pin | 3.3 V / 0 V / 1 Hz square wave | `gpioset all=1` / `all=0` / `--blink all` |
