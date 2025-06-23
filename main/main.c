#include "MicroUSC/USCdriver.h"
#include "esp_log.h"
#include "testing_driver.h"
#include "speed_test.h"
#include "esp_wifi.h"

void app_main(void) {
    // microusc_start_wifi("user", "password");
    init_MicroUSC_system();

    uart_config_t setting = STANDARD_UART_CONFIG; // only for debugging
    /*
    uart_port_config_t pins = {
        .tx = GPIO_NUM_17,
        .rx = GPIO_NUM_16,
        .port = UART_NUM_2
    };
    */

    uart_port_config_t pins = {
        .port = UART_NUM_2, // make it to 1
        .rx = GPIO_NUM_16, // 17
        .tx = GPIO_NUM_17 // 18
    };

    // code should go after here
    
    usc_process_t driver_action = &system_task; // point to the function you created
    // function will configure driver_example

    // uncomment the line below to test the speed of the function
    usc_driver_install("first driver", setting, pins, driver_action, 4086);
    
    /*
    uart_port_config_t pinss = {
        .tx = GPIO_NUM_4,
        .rx = GPIO_NUM_5,
        .port = UART_NUM_1
    };

    CHECK_FUNCTION_SPEED_WITH_DEBUG(usc_driver_install("second driver", setting, pinss, driver_action));
    */

    send_microusc_system_status(USC_SYSTEM_LED_ON);
    send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);
    send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
    
    printf("Pausing system...\n");
    send_microusc_system_status(USC_SYSTEM_PAUSE);
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    send_microusc_system_status(USC_SYSTEM_LED_OFF);
    //vTaskDelay(4000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    //send_microusc_system_status(USC_SYSTEM_RESUME);
    //vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for the system to be ready (1 second)
    //send_microusc_system_status(USC_SYSTEM_ERROR);
    //send_microusc_system_status(USC_SYSTEM_SLEEP);
    microusc_system_start_mqtt_service("mqtt://192.168.1.23");
    printf("End of program\n");
}