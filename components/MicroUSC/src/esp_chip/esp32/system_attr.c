#include "MicroUSC/chip_specific/system_attr.h"
#include "driver/gpio.h"

#define BLINK_GPIO GPIO_NUM_2

void init_builtin_led(void) 
{
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
}

void builtin_led_set(bool state) 
{
    gpio_set_level(GPIO_NUM_2, state);
}

void builtin_led_system(microusc_status status)
{
    switch (status) {
        case USC_SYSTEM_LED_ON:
            builtin_led_set(1);
            break;
        case USC_SYSTEM_LED_OFF:
            builtin_led_set(0);
            break;
        default:
            builtin_led_set(0);
            break;
    }
}