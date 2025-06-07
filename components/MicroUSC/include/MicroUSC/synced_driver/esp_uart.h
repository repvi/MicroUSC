/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file esp_uart.h
 * @brief UART driver abstraction layer for ESP32/ESP8266 embedded systems
 *
 * Provides a high-level API for configuring and managing UART peripherals on ESP32/ESP8266,
 * integrating FreeRTOS queues for interrupt-driven communication. Designed for robust serial
 * I/O in real-time embedded applications.
 *
 * Features:
 * - UART port initialization with GPIO pin mapping
 * - Thread-safe data reception via FreeRTOS queues
 * - Timeout handling for real-time systems
 * - Hardware resource cleanup and validation
 *
 * Usage:
 * 1. Initialize UART with uart_init()
 * 2. Read data using uart_read() with queue-based event handling
 * 3. Deinitialize configurations with uart_port_config_deinit() during shutdown
 *
 * @note Part of the MicroUSC system codebase for ESP32/ESP8266 development
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

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

/**
 * @struct uart_port_config_t
 * @brief Configuration structure for UART ports.
 */
typedef struct {
  uart_port_t port; ///< UART port identifier.
  gpio_num_t tx;  ///< GPIO pin for UART transmit.
  gpio_num_t rx;  ///< GPIO pin for UART receive.
} uart_port_config_t;

/**
 * @brief Initialize a UART port with specified configurations and pins.
 * 
 * This function configures a UART peripheral on ESP32 with the given port settings, 
 * GPIO pin assignments, and creates a FreeRTOS queue for interrupt-driven event handling.
 * Combines ESP-IDF's `uart_param_config`, `uart_set_pin`, and `uart_driver_install` into a single call.
 * 
 * @param port_config Contains UART port number (e.g., UART_NUM_0) and TX/RX/RTS/CTS pins
 * @param uart_config Baud rate, data bits, parity, stop bits, and flow control settings
 * @param queue_size Number of events the queue can hold (min 1 for interrupt operation)
 * @return 
 * - ESP_OK: UART initialized successfully
 * - ESP_ERR_INVALID_ARG: Invalid port/config parameters
 * - ESP_FAIL: Driver installation failed
 * 
 * @note Must be called once per UART port during system initialization.
 *       Ensure GPIO pins are valid for the target ESP32 variant.
 *       Use IRAM_ATTR for ISR handlers if UART used in interrupt context.
 */
void uart_init( uart_port_config_t port_config, 
                uart_config_t uart_config
              );



uint8_t *uart_offset_repair( uart_port_t uart, 
                             uint8_t *buf,
                             const size_t len, 
                             const TickType_t delay
                           );
/**
 * @brief Read data from a UART port using FreeRTOS queues with timeout.
 *
 * This function reads up to `len` bytes from the specified UART port into the provided buffer,
 * utilizing FreeRTOS queue-based event handling for real-time data reception. Designed for
 * ESP32/ESP8266 embedded systems with strict timing requirements.
 *
 * @param uart UART port number (e.g., UART_NUM_0)
 * @param buf Pre-allocated buffer for storing received data
 * @param len Maximum number of bytes to read
 * @param delay Maximum ticks to wait for data (use portMAX_DELAY for blocking)
 * @return uint8_t* - Pointer to buffer with received data, NULL on timeout/error
 *
 * @note Buffer must be allocated before calling. Queue must be initialized
 *       via uart_init() first. Suitable for interrupt-driven designs when
 *       combined with proper ISR configuration.
 */
/**
 * @brief Read data from a UART port using FreeRTOS queues with timeout.
 *
 * This function reads up to `len` bytes from the specified UART port into the provided buffer,
 * utilizing FreeRTOS queue-based event handling for real-time data reception. Designed for
 * ESP32/ESP8266 embedded systems with strict timing requirements.
 *
 * @param uart UART port number (e.g., UART_NUM_0)
 * @param buf Pre-allocated buffer for storing received data
 * @param len Maximum number of bytes to read
 * @param delay Maximum ticks to wait for data (use portMAX_DELAY for blocking)
 * @return uint8_t* - Pointer to buffer with received data, NULL on timeout/error
 *
 * @note Buffer must be allocated before calling. Queue must be initialized
 *       via uart_init() first. Suitable for interrupt-driven designs when
 *       combined with proper ISR configuration.
 */
uint8_t *uart_read( uart_port_t uart,
                    uint8_t *buf,
                    const size_t len, 
                    const TickType_t delay
                  );

/**
 * @brief Deinitialize a UART port configuration structure.
 *
 * This function resets the fields of a uart_port_config_t structure to indicate that
 * it is no longer associated with any valid UART port or GPIO pins. It sets the port
 * to UART_NUM_MAX (an invalid port) and both RX and TX pins to GPIO_NUM_NC (not connected).
 *
 * Typical use: Call this function to safely mark a UART port configuration as unused
 * or before reconfiguring it for a different port or pin assignment in embedded systems.
 *
 * @param uart_config Pointer to the uart_port_config_t structure to be deinitialized.
 *
 * @note This helps prevent accidental reuse of stale or invalid UART configurations,
 *       supporting robust system and hardware interface management in ESP32 applications.
 */
/**
 * @brief Deinitialize a UART port configuration structure.
 *
 * This function resets the fields of a uart_port_config_t structure to indicate that
 * it is no longer associated with any valid UART port or GPIO pins. It sets the port
 * to UART_NUM_MAX (an invalid port) and both RX and TX pins to GPIO_NUM_NC (not connected).
 *
 * Typical use: Call this function to safely mark a UART port configuration as unused
 * or before reconfiguring it for a different port or pin assignment in embedded systems.
 *
 * @param uart_config Pointer to the uart_port_config_t structure to be deinitialized.
 *
 * @note This helps prevent accidental reuse of stale or invalid UART configurations,
 *       supporting robust system and hardware interface management in ESP32 applications.
 */
void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif