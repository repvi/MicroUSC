#include "MicroUSC/USCdriver.h"
#include "testing_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "stddef.h"
#include "stdint.h"
#include <inttypes.h>

/* n = 0 -> default value 
   n = 1 -> turn off esp32
   n = 2 -> sleep mode
   n = 3 -> pause port data
   n = 4 -> connect to wifi
   n = 5 -> connect to bluetooth
   n = 6 -> turn on built-in LED on esp32
   n = 7 -> memory usage 
   n = 8 -> system specs
   n = 9 -> driver status
   n = 10 -> overdriver status
*/

void system_task(void *p) {
    uscDriverHandler driver = (uscDriverHandler)p;
    uint32_t data = 0;

    while (1) {
        data = usc_driver_get_data(driver);

        if (data != 0) {
            ESP_LOGI("driver task", "Got data: %lu", data);
        }
        
        switch(data) {
            case 1: 
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
                break;
            case 7:
                break;
            case 8:
                break;
            case 9:
                break;
            case 10:
                break;
        }

        vTaskDelay(portTICK_PERIOD_MS); // delay for one second
        // this is used to delay the task for a certain amount of time
        // it is used to prevent the task from running too fast and consuming too much CPU time
        // Used to reset the timer for the task
        printf("Running system task...\n"); // Debug message to show the task is running
    }
}






















