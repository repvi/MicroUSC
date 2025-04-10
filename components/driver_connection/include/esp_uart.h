#ifndef __ESP_UART_SETTING_H
#define __ESP_UART_SETTING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

#define TIMEOUT pdMS_TO_TICKS     (100)

/**
 * @struct uart_port_config_t
 * @brief Configuration structure for UART ports.
 */
typedef struct {
    gpio_num_t tx;  ///< GPIO pin for UART transmit.
    gpio_num_t rx;  ///< GPIO pin for UART receive.
    uart_port_t port; ///< UART port identifier.
} uart_port_config_t;

typedef int baud_rate_t;

esp_err_t uart_init(uart_port_config_t port_config, uart_config_t uart_config);

char *uart_read(uart_port_t *uart, size_t len);

uint8_t *uart_read_u(uart_port_t *uart, size_t len, TickType_t timeout);

void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif

#endif