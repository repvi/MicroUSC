#include "USC_driver.h"
#include "nvs_flash.h" // doesn't need to be included, recommended to have
//IRAM_ATTR

// git log
// git checkout [c50cad7fbea3ae70313ac72c68d59a8db20e8dc8]
// git commit -m 'Change details'
// git branch 
// git checkout -b [name of branch]
// git push origin [name of branch]
// git pull origin master
// git status
// git switch [name of branchgit ]           switches to another branch
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);    
    
    uart_config_t setting = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_port_config_t pins = {
        .tx = GPIO_NUM_1,
        .rx = GPIO_NUM_3,
        .port = UART_NUM_0
    };

    // code should go after here

    // functions like init_[something([varaible type] &configuration);

    usc_config_t driver_example = {
        .uart_config = pins,
        .driver_name = "Driver example"
    };

    // make sure this points to a function that will handle
    // the actions of the serial code that is recieved.
    /* For example, create 
    function_driver(void *parameter) {
        [type] *something = ([type] *)parameter;
        while (1) {
            // do something
            if ([data] equals example) {
                // do action
            }
        }
        // data can be NULL so make sure to add safe code using
        // if statements
    }
    */
    
    usc_data_process_t driver_action = NULL; // point to the function
    // you created
    
    
    // function will configure driver_example
    // 0 is for the driver type, for now you can only use 0 and 1.
    // do not use the same number or it will not be configured

    ret = usc_driver_init(&driver_example, setting, &driver_action, 0);
    if (ret != ESP_OK) { // temporary
        printf("something went wrong here\n");
        return;
    }
}