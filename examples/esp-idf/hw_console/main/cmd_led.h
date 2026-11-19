#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* led: the two on-board user LEDs (GPIO2 / GPIO3, active low) */
void register_led_commands(void);

#ifdef __cplusplus
}
#endif
