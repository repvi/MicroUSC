#include "MicroUSC-internal.h"
#include "MicroUSC-kernel.h"
#include "USCdriver.h"

/* n = 0  -> default value
   n = 1  -> turn off esp32
   n = 2  -> sleep mode
   n = 3  -> pause port data
   n = 4  -> connect to wifi
   n = 5  -> connect to bluetooth
   n = 6  -> turn on built-in LED on esp32
   n = 7  -> turn off built-in led on esp32
   n = 8  -> memory usage
   n = 9  -> system specs
   n = 10 -> driver status
*/

#define BLINK_GPIO GPIO_NUM_2

int __microusc_system_code = 0;

void __microsc_system_task(void *p)
{
    while (1) {
         switch(__microusc_system_code) {
            case 1:
                esp_restart();
                break;
            case 2:
                break;
            case 3:

                break;
            case 4:
                break;
            case 5:
                break;
            case 6:
                gpio_set_level(BLINK_GPIO, 1); // ON
                break;
            case 7:
                gpio_set_level(BLINK_GPIO, 0); // OFF
                break;
            case 8:
                break;
            case 9:
                break;
        }
    }
    
}

esp_err_t microusc_system_task(void)
{
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    xTaskCreatePinnedToCore(
        __microsc_system_task,
        "microUSB System",
        TASK_STACK_SIZE,
        NULL,
        MICROUSC_SYSTEM_PRIORITY,
        NULL,
        MICROUSC_CORE
    );
    return ESP_OK;
}