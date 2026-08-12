# ES8389 I2S loopback

This example initializes the on-board ES8389 codec and continuously forwards the two differential microphone channels to the speaker output.

It uses the board variant definitions for I2C, I2S, and `PA_POWER`. The ES8389 uses Arduino Wire's 7-bit I2C address `0x10` (the ESP-IDF codec component represents it as `0x20`). The codec runs as an I2S slave at 48000 Hz, 16-bit stereo, using its internal clock; GPIO13 is not configured as MCLK. Open Serial Monitor at 115200 baud to see initialization status.

Keep the speaker volume low during the first run to avoid acoustic feedback.
