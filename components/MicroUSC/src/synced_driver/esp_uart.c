#include "MicroUSC/synced_driver/esp_uart.h"
#include "esp_heap_caps.h"
#include <string.h>

#define TAG "[UART]"

#define DLEAY_MILISECOND_10      (10) // 1 second delay
#define DLEAY_MILISECOND_50      (50) // 1 second delay

#define TIMEOUT pdMS_TO_TICKS     (100)

#define BUFFER_SIZE               (256)

#define xQueCreateSet(x) xQueueCreate(x, sizeof(uart_event_t))

void uart_init( uart_port_config_t port_config, 
                uart_config_t uart_config, 
                QueueHandle_t *uart_queue, 
                const size_t queue_size
) {
    ESP_ERROR_CHECK(uart_param_config(port_config.port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port_config.port, port_config.tx, port_config.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port_config.port, BUFFER_SIZE * 2, 0, queue_size, uart_queue, 0));
}

uint8_t *uart_read( uart_port_t uart, 
                    uint8_t *buf,
                    const size_t len, 
                    QueueHandle_t uart_queue,
                    const TickType_t delay
) {
    if (buf == NULL) {
        ESP_LOGE(TAG, "Buffer is not initialized");
        return NULL;
    }

    memset(buf, 0, len); // make sure buf is clean

    uart_event_t event;
    size_t total_size = 0;
    uint8_t *ptr = buf;
    int timeout_count = 0;

    while (total_size < len) {
        if (xQueueReceive(uart_queue, &event, delay)) {
            switch (event.type) {
                case UART_DATA: {
                    int size = uart_read_bytes(uart, ptr, event.size, delay);
                    if (size <= 0) {
                        timeout_count++;
                        if (timeout_count >= 6) {
                            break; // Return what we have so far
                        }
                        continue;
                    }
                    // Optionally, check for a delimiter (e.g., newline)
                    for (int i = 0; i < size; i++) {
                        if (ptr[i] == '\n') { // Use your protocol's delimiter
                            total_size += i + 1;
                            goto done;
                        }
                    }
                    ptr += size;
                    total_size += size;
                    break;
                }
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(uart);
                    xQueueReset(uart_queue);
                    break;
                default:
                    break;
            }
        }
        else if (total_size == 0) {
            break;
        }
    }
done:
    // Ensure null-termination if you expect text
    if (total_size < len) buf[total_size] = '\0';
    else buf[len - 1] = '\0';
    return buf;
}

void uart_port_config_deinit(uart_port_config_t *uart_config) 
{
    uart_config->port = UART_NUM_MAX; // Not a real PORT
    uart_config->rx = GPIO_NUM_NC; // -1
    uart_config->tx = GPIO_NUM_NC; // -1
}