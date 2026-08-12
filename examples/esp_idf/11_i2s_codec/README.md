# ES8389 / ES8311 I2S Codec

[中文版本](./README_CN.md)

Run continuous PCM playback or microphone echo through an I2S audio codec.
On ESP32-P4, the checked-in defaults enable the local board BSP and use its
ES8389 playback and recording devices. The non-BSP reference path remains
available for an ES8311 wired directly to the configured pins.

## Hardware Required

- ESP32-P4-WIFI6-DB-TOUCH-LCD-10.1 board for the default BSP/ES8389 path.
- Speaker or headphones for playback.
- Microphone input for echo mode.

## Build and Flash

```powershell
cd examples\esp_idf\10_i2s_codec
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the actual serial port.

## Configuration

Open **Example Configuration**:

- Select `music` to loop the embedded `main/canon.pcm` sample.
- Select `echo` to read microphone PCM and write it back to the output.
- Set **Voice volume** from 0 to 100.
- Keep **Enable Board Support Package (BSP) support** enabled for this board.

The Kconfig default is `music`, and `sdkconfig.defaults` enables the BSP. Both
music and echo modes have BSP code paths. The microphone-gain menu controls the
non-BSP ES8311 path; the current BSP helper configures the ES8389 input gain to
24 dB.

The board BSP initializes I2S RX/TX in stereo mode. After opening the ES8389
recording device, it configures the two differential microphone inputs through
`bsp_audio_codec_configure_microphone()`: registers 0x72 and 0x73 select the
differential input paths, and the same PGA gain is applied to both channels.
Call the BSP function again after changing the microphone gain.

Current ESP32-P4 I2C/I2S wiring:

| Signal | GPIO |
| --- | ---: |
| I2C SDA | 7 |
| I2C SCL | 8 |
| I2S MCLK | 13 |
| I2S BCLK | 12 |
| I2S WS | 10 |
| I2S DOUT | 9 |
| I2S DIN | 11 |
| Power amplifier enable | 53 |

## Expected Behavior

- BSP startup logs `Using ESP32-P4 BSP for I2S and codec configuration`.
- Successful initialization logs `i2s driver init success` and
  `es8311 codec init success`. The second message is a legacy function/log name;
  the ESP32-P4 BSP path initializes ES8389.
- Music mode repeatedly logs the number of PCM bytes written.
- Echo mode logs `[echo] Echo start`, then continuously captures and plays PCM.
- Echo mode logs `[echo] captured peak: left=..., right=... (PCM16)` once per second.

Initialization logs prove the software path opened. They do not prove that
either PCM channel contains a valid microphone signal or that the analog output
is audible.

## Troubleshooting

- Confirm codec power, MCLK/BCLK/WS activity, I2C control, I2S wiring, PA
  enable, and the selected speaker/headphone route.
- In echo mode, inspect left and right PCM peak or RMS values before assigning
  the fault to the codec or analog hardware.
- If the BSP is disabled, the reference path expects ES8311 rather than
  ES8389.
- Use [02_i2c_console_test](../02_i2c_console_test/) to confirm that the
  expected codec responds on GPIO7/GPIO8.

The README has been checked against the source. Audio output and capture remain
to be verified on hardware.
