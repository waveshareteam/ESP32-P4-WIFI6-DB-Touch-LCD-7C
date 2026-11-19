#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* twai_init / twai_deinit / twai_send / twai_dump / twai_info / twai_recover
 * on the on-board CAN transceiver (same command style as examples/esp-idf/13_twai) */
void register_twai_commands(void);

#ifdef __cplusplus
}
#endif
