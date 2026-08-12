#pragma once

#include "esp_err.h"
#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t lcd_ek79007_init(esp_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif
