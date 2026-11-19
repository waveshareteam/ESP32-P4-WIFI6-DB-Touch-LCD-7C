#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mount / umount / df / ls / dd (SD card and USB MSC share the file commands) */
void register_storage_commands(void);

bool storage_sd_is_mounted(void);

#ifdef __cplusplus
}
#endif
