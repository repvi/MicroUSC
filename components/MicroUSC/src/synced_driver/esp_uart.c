#include "MicroUSC/synced_driver/esp_uart.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include <string.h>

static const char *TAG = "[UART]";

#define DELAY_MILISECOND_10      (10) // 1 second delay
#define DELAY_MILISECOND_50      (50) // 1 second delay

#define TIMEOUT (pdMS_TO_TICKS(100))

#define MINIMUM_BUFFER_RX_SIZE   16

esp_err_t uart_init( uart_port_config_t port_config, 
                     uart_config_t uart_config
) {
    ESP_RETURN_ON_ERROR(uart_param_config(port_config.port, &uart_config), TAG, "UART param config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(port_config.port, port_config.tx, port_config.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "UART set pin failed");
    ESP_RETURN_ON_ERROR(uart_driver_install(port_config.port, BUFFER_SIZE, 0, 0, NULL, 0), TAG, "UART driver install failed");
    return ESP_OK;
}

uint8_t *uart_offset_repair( uart_port_t uart, 
                                uint8_t *buf,
                                const size_t len, 
                                const TickType_t delay
) {
    size_t size_of_rx_buffer = 0;
    esp_err_t status;

    do {
        int res = uart_read_bytes(uart, buf, 1, delay); // one byte offset each time
        if (res == -1) {
            //uart_flush_input(uart);
            break;
        }
        else if (buf[0] == 0xFF) {
            status = uart_get_buffered_data_len(uart, &size_of_rx_buffer);
            if (status == ESP_OK && size_of_rx_buffer >= (len - 1)) {
                // read rest of frame
                res = uart_read_bytes(uart, buf + 1, len - 1, delay);
                if (res != -1 && buf[1 + sizeof(uint32_t)] == 0xFF) {
                    // successfully repaired
                    return buf;
                }
            }
        }
        uart_get_buffered_data_len(uart, &size_of_rx_buffer);
    } while (size_of_rx_buffer != 0);
    buf[0] = 0xFF;
    buf[1 + sizeof(uint32_t)] = 0xFF;
    return NULL;
}

uint8_t *uart_read( uart_port_t uart, 
                    uint8_t *buf,
                    const size_t len, 
                    const TickType_t delay
) {
    size_t size_of_rx_buffer;
    esp_err_t err = uart_get_buffered_data_len(uart, &size_of_rx_buffer);
    ESP_RETURN_ON_FALSE(err == ESP_OK, NULL, TAG, "Failed to get buffered data length" );
    if (size_of_rx_buffer < len) {
        ESP_LOGE(TAG, "Returning less than len: %u", size_of_rx_buffer);
        return NULL;
    }

    int status = uart_read_bytes(uart, buf, len, delay);
    if (status != -1 && (buf[0] != 0xFF || buf[1 + sizeof(uint32_t)] != 0xFF)) {
        ESP_LOGE(TAG, "Recieved: %u %u %u %u %u %u", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        return uart_offset_repair(uart, buf, len, delay);
    }
    return buf;
}

void uart_port_config_deinit(uart_port_config_t *uart_config) 
{
    uart_config->port = UART_NUM_MAX; // Not a real PORT
    uart_config->rx = GPIO_NUM_NC; // -1
    uart_config->tx = GPIO_NUM_NC; // -1
}