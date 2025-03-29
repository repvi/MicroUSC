#include "USC_driver.h"
#include "nvs_flash.h" // doesn't need to be included, recommended to have
//IRAM_ATTR

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
        .tx = GPIO_NUM_3,
        .port = UART_NUM_0
    };

    // code should go after here

    // functions like init_[something([varaible type] &configuration);
}