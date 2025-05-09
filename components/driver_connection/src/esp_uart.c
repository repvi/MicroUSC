#include "esp_uart.h"
#include "esp_heap_caps.h"
#include <string.h>

#define DLEAY_MILISECOND_10      (10) // 1 second delay
#define DLEAY_MILISECOND_50      (50) // 1 second delay

esp_err_t uart_init(uart_port_config_t port_config, uart_config_t uart_config, QueueHandle_t uart_queue) {
    esp_err_t res = uart_param_config(port_config.port, &uart_config);
    if (res == ESP_OK) {
        res = uart_set_pin(port_config.port, port_config.tx, port_config.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (res == ESP_OK) {
            res = uart_driver_install(port_config.port, BUFFER_SIZE * 2, 0, 0, &uart_queue, 0);
        }
    }
    return res;
}

uint8_t *uart_read(const uart_port_t *uart, const size_t len, QueueHandle_t uart_queue) {
    uart_event_t event;
    uint8_t *buf = (uint8_t *)heap_caps_malloc(len, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (buf == NULL) { // failed to allocate memory
        ESP_LOGE("uart_read", "Failed to allocate memory for buffer");
        return NULL;
    }

    memset(buf, '\0', len); // Clear the buffer

    uint8_t *ptr = buf;
    int total_size = 0;
    int timeout_count = 0;

    while (total_size < BUFFER_SIZE) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    int size = uart_read_bytes(*uart, buf, event.size, portMAX_DELAY);

                    if (size <= 0) {
                        timeout_count++;
                        if (timeout_count >= 6) {
                            return buf;
                        }
                        continue;
                    }
            
                    total_size += size;
                    ptr += size;
            
                    // Check for newline character in the newly read data
                    for (uint32_t i = total_size - size; i < total_size; i++) {
                        if (buf[i] == '\0') {
                            return buf;    // Return the buffer on success
                        }
                    }
                    break;
                case UART_FIFO_OVF:                     // Handle FIFO overflow
                    uart_flush_input(*uart);
                    xQueueReset(uart_queue);
                    break;
                default:
                    break;
            }
        }

        #if (INCLUDE_DELAY == 1)
        vTaskDelay(DLEAY_MILISECOND_50 / portTICK_PERIOD_MS); // 10ms delay
        #endif
    }

    buf[total_size - 1] = '\0'; // make sure the last byte is null-terminated
    return buf;
}

void uart_port_config_deinit(uart_port_config_t *uart_config) {
    uart_config->port = UART_NUM_MAX; // Not a real PORT
    uart_config->rx = GPIO_NUM_NC; // -1
    uart_config->tx = GPIO_NUM_NC; // -1
}

#undef INCLUDE_DELAY