#include "driver/gpio.h"
#include "esp_err.h"

#define USER_LED_1_GPIO     GPIO_NUM_2
#define USER_LED_2_GPIO     GPIO_NUM_3
#define USER_LED_ON_LEVEL   0   /* The user LEDs are active low. */

void app_main(void)
{
    const gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << USER_LED_1_GPIO) | (1ULL << USER_LED_2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&led_config));

    ESP_ERROR_CHECK(gpio_set_level(USER_LED_1_GPIO, USER_LED_ON_LEVEL));
    ESP_ERROR_CHECK(gpio_set_level(USER_LED_2_GPIO, USER_LED_ON_LEVEL));
}
