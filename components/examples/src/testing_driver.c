#include "MicroUSC/USCdriver.h"
#include "testing_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "stddef.h"
#include "stdint.h"
#include <inttypes.h>

void system_task(void *p) {
    uscDriverHandler driver = (uscDriverHandler)p;
    uint32_t data = 0;

    while (1) {
        data = usc_driver_get_data(driver);

        if (data != 0) {
            ESP_LOGI("driver task", "Got data: %lu", data);
            if (data == 0x64) {
                usc_send_data(driver, 1234); // send password
            }
            else {
                usc_send_data(driver, data + 1); // increment by 1
            }
        }
        
        switch(data) {
            case 1: 
                break;
            case 2:
                break;
            case 3: 
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // delay for one second
        // this is used to delay the task for a certain amount of time
        // it is used to prevent the task from running too fast and consuming too much CPU time
        // Used to reset the timer for the task
        printf("Running system task...\n"); // Debug message to show the task is running
    }
}






















