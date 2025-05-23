#ifndef __ESP_UART_SETTING_H
#define __ESP_UART_SETTING_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIMEOUT pdMS_TO_TICKS     (100)

#define INCLUDE_DELAY             (0)

#define BUFFER_SIZE               (256)

#define UART_QUEUE_SIZE           (10)

#define INIT_DEBUG_PORT_CONFIG { \
    .port = UART_NUM_0, \
    .tx   = GPIO_NUM_1, \
    ,rx   = GPIO_NUM_3  \
  } \

#ifdef CONFIG_IDF_TARGET_ESP32
#define INIT_STANDARD_PORT2_CONFIG { \
    .port = UART_NUM_1,  \
    .tx   = GPIO_NUM_17, \
    .rx   = GPIO_NUM_16  \
  }
#endif

#if CONFIG_IDF_TARGET_ESP32S3
#define INIT_STANDARD_PORT1_CONFIG { \
    .port = UART_NUM_1,  \
    .tx   = GPIO_NUM_17, \
    .rx   = GPIO_NUM_18  \
  }
#endif

#define xQueCreateSet(x) xQueueCreate(x, sizeof(uart_event_t))
/**
 * @struct uart_port_config_t
 * @brief Configuration structure for UART ports.
 */
typedef struct {
  uart_port_t port; ///< UART port identifier.
  gpio_num_t tx;  ///< GPIO pin for UART transmit.
  gpio_num_t rx;  ///< GPIO pin for UART receive.
} uart_port_config_t;

esp_err_t uart_init( uart_port_config_t port_config, 
                     uart_config_t uart_config, 
                     QueueHandle_t *uart_queue, 
                     const size_t queue_size
                   );

uint8_t *uart_read( uart_port_t uart,
                    uint8_t *buf,
                    const size_t len, 
                    QueueHandle_t uart_queue,
                    const TickType_t delay
                  );

void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif

#endif