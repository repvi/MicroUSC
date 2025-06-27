#include "MicroUSC/system/manager_isr.h"
#include "MicroUSC/system/manager.h"

void IRAM_ATTR microusc_software_isr_handler(void *arg)
{
    MiscrouscBackTrack_t current;
    current.status = *(microusc_status *)arg;
    xQueueSendFromISR(microusc_system.queue_system.queue_handler, &current, NULL);
}

/* Add on the next version */
__deprecated void microusc_system_isr_trigger(void) 
{
    /* TODO */
}

void microusc_system_isr_pin(gpio_config_t io_config, microusc_status trigger_status)
{
    int bit_mask = io_config.pin_bit_mask;
    int gpio_pin = 1;

    /* Determine the GPIO pin number from the bit mask */
    if (bit_mask == 0) return;
    while (bit_mask > 1) {
        bit_mask >>= 1;
        gpio_pin++;
    }

    /* Remove any existing ISR handler for this pin */
    gpio_isr_handler_remove(gpio_pin);

    /* Configure the GPIO pin with the provided settings */
    gpio_config(&io_config);

    /* Add the new ISR handler for this pin */
    gpio_isr_handler_add((gpio_num_t)gpio_pin, microusc_software_isr_handler, &gpio_pin);
}