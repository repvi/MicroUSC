#include "MicroUSC/synced_driver/esp_uart.h"
#include "esp_heap_caps.h"
#include <string.h>

#define TAG "[UART]"

#define DLEAY_MILISECOND_10      (10) // 1 second delay
#define DLEAY_MILISECOND_50      (50) // 1 second delay

#define TIMEOUT pdMS_TO_TICKS     (100)

#define MINIMUN_BUFFER_RX_SIZE   16

void uart_init( uart_port_config_t port_config, 
                uart_config_t uart_config, 
) {
    ESP_ERROR_CHECK(uart_param_config(port_config.port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port_config.port, port_config.tx, port_config.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port_config.port, BUFFER_SIZE * 2, 0, 0, NULL, 0));
}

uint8_t *uart_read( uart_port_t uart, 
                    uint8_t *buf,
                    const size_t len, 
                    const TickType_t delay
) {
    if (uart_get_buffered_data_len(uart) < len) {
        return NULL;
    }

    uint8_t *ptr = buf;
    uart_read_bytes(uart, ptr, len, delay);
    return buf;
}

void uart_port_config_deinit(uart_port_config_t *uart_config) 
{
    uart_config->port = UART_NUM_MAX; // Not a real PORT
    uart_config->rx = GPIO_NUM_NC; // -1
    uart_config->tx = GPIO_NUM_NC; // -1
}