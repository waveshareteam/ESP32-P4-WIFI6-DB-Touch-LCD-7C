#include <stdio.h>
#include "bsp/esp-bsp.h"

void app_main(void)
{
    bsp_led_init();
    bsp_led_set(BSP_LED_USER1, true);
    bsp_led_set(BSP_LED_USER2, true);
}
