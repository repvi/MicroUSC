#include "testing_driver.h"
#include "string.h"
#include "stddef.h"
#include "stdint.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
    Command List:
    0 -> default value
    1 -> turn off ESP32
    2 -> deep sleep mode
    3 -> pause port data
    4 -> connect to WiFi
    5 -> connect to Bluetooth
*/

static const char *TAG = "CMD_HANDLER";

uint32_t data_returner(void) {
    return 2;  // Simulate command (e.g., deep sleep)
}

void get_data(void *p) {
    uint32_t data = 0;

    while (1) {
        data = data_returner();

        switch (data) {
            case 1:
                ESP_LOGI(TAG, "Shutting down ESP32");
                break;
            case 2:
                ESP_LOGI(TAG, "ESP entering deep sleep mode");
                esp_sleep_enable_timer_wakeup(10 * 1000000);
                esp_deep_sleep_start();
                break;
            case 3:
                ESP_LOGI(TAG, "Pausing port data");
                break;
            case 4:
                ESP_LOGI(TAG, "Connecting to WiFi");
                break;
            case 5:
                ESP_LOGI(TAG, "Connecting to Bluetooth");
                break;
            default:
                ESP_LOGW(TAG, "Unknown command: %d", data);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
