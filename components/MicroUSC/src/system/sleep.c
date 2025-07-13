#include "MicroUSC/system/sleep.h"
#include "esp_sleep.h"

struct sleep_config_t {
    gpio_num_t wakeup_pin;
    uint64_t time;
    bool wakeup_pin_enable;
    bool sleep_time_enable;
} deep_sleep;

void sleep_mode_timer_wakeup(uint64_t time) 
{
    deep_sleep.time = time;
}

void sleep_mode_timer(bool option) 
{
    deep_sleep.sleep_time_enable = option;
}
 
void sleep_mode_wakeup_pin(gpio_num_t pin) 
{
    deep_sleep.wakeup_pin = pin;
}
 
void sleep_mode_wakeup_pin_status(bool option)
{
    deep_sleep.wakeup_pin_enable = option;
}

void sleep_mode_wakeup_default(void) 
{
    sleep_mode_timer_wakeup(DEFAULT_LIGHTMODE_TIME);
    sleep_mode_timer(true);
    sleep_mode_wakeup_pin(GPIO_NUM_NC);
    sleep_mode_wakeup_pin_status(false);
}

void sleep_mode(void)
{
    if (!(deep_sleep.sleep_time_enable || deep_sleep.wakeup_pin_enable)) {
        return; // do nothing as timer is completely disabled
    }

    if (deep_sleep.sleep_time_enable) {
        esp_sleep_enable_timer_wakeup(deep_sleep.time);
    }
    if (deep_sleep.wakeup_pin_enable) {
        esp_sleep_enable_ext0_wakeup(deep_sleep.wakeup_pin, 1);
    }
    esp_deep_sleep_start();
}