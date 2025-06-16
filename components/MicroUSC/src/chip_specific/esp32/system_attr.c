#include "MicroUSC/chip_specific/system_attr.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"

#define BUILTIN_LED GPIO_NUM_2

/* Defined in asssembly file */
extern void turn_on_builtin_led(void);
extern void turn_off_builtin_led(void);

// #define turn_on_builtin_led() REG_SET_BIT(GPIO_OUT_REG, BIT(BUILTIN_LED));
// #define turn_off_builtin_led() REG_CLR_BIT(GPIO_OUT_REG, BIT(BUILTIN_LED));

/*
 REG_WRITE(GPIO_FUNC0_OUT_SEL_CFG_REG + BUILTIN_LED * 4, 256); // 256 = Direct GPIO control
    // Enable GPIO as an output
    REG_SET_BIT(GPIO_ENABLE_REG, BIT(BUILTIN_LED));
    turn_off_builtin_led(); // make sure it is off by default
*/

void builtin_led_set(bool state)
{
    //gpio_set_level(GPIO_NUM_2, state);
}

__init void init_builtin_led_wrapper(void)
{
    init_builtin_led();
}

void builtin_led_system(microusc_status status)
{
    if (status != USC_SYSTEM_LED_ON) {
        turn_off_builtin_led();
    }
    else {
        turn_on_builtin_led();
    }
}