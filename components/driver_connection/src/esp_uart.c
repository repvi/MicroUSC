#include "esp_uart.h"
#include "esp_heap_caps.h"
#include <string.h>

#define TIMEOUT pdMS_TO_TICKS     (100)
#define BUFFER_SIZE               (256)

/*
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
*/

esp_err_t uart_init(uart_port_config_t port_config, uart_config_t uart_config) {
    esp_err_t res = uart_param_config(port_config.port, &uart_config);
    if (res == ESP_OK) {
        res = uart_set_pin(port_config.port, port_config.tx, port_config.rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (res == ESP_OK) {
            res = uart_driver_install(port_config.port, BUFFER_SIZE * 2, 0, 0, NULL, 0);
        }
    }

    return res;
}

serial_data_ptr_t uart_read(uart_port_t *uart, size_t len) {
    serial_data_ptr_t buf = (serial_data_ptr_t)heap_caps_malloc(len, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (buf == NULL) { // failed to allocate memory
        ESP_LOGE("uart_read", "Failed to allocate memory for buffer");
        return NULL;
    }

    memset(buf, '\0', len); // Clear the buffer

    serial_data_ptr_t ptr = buf;
    int total_size = 0;
    int timeout_count = 0;
    
    while (total_size < BUFFER_SIZE) {
        int size = uart_read_bytes(*uart, ptr, len - total_size, TIMEOUT); // needs to slow down
        
        if (size < 0) {
            timeout_count++;
            if (timeout_count >= 6) {
                return buf;
            }
            continue;
        }

        total_size += size;
        ptr += size;

        // Check for newline character in the newly read data
        for (int i = total_size - size; i < total_size; i++) {
            if (buf[i] == '\n') {
                buf[i] = '\0'; // Null-terminate the string
                return buf;    // Return the buffer on success
            }
        }
    }

    buf[total_size - 1] = '\0';
    return buf;
}