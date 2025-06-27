#pragma once

#include "MicroUSC/system/uscsystemdef.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure a GPIO pin as an interrupt source for the MicroUSC system and register an ISR.
 *
 * This function determines the GPIO pin from the provided gpio_config_t's pin_bit_mask,
 * removes any existing ISR handler for that pin, configures the pin with the given settings,
 * and attaches the specified ISR handler. It also sets the status code that will be triggered
 * when the ISR is called.
 *
 * Typical usage: Call this during system initialization to set up a wakeup or event pin.
 *
 * @param io_config      GPIO configuration structure specifying the pin and settings.
 * @param trigger_status The MicroUSC status code to associate with this ISR event.
 */
void microusc_system_isr_pin(gpio_config_t io_config, microusc_status trigger_status);



#ifdef __cplusplus
}
#endif