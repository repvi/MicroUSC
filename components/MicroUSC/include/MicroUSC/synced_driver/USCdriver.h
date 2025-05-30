/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file USCdriver.h
 * @brief MicroUSC UART Driver API for ESP32/ESP8266 Embedded Systems
 *
 * This header provides the main interface for initializing, managing, and communicating with UART-based drivers
 * in the MicroUSC system on ESP32/ESP8266 platforms. It integrates with FreeRTOS, memory pools, and atomic
 * operations for robust embedded system design, and is intended to be included in modules that require driver
 * configuration, data transmission, and secure communication.
 *
 * Key Features:
 * - UART driver initialization with configurable baud rates, pins, and callbacks
 * - Thread-safe data transmission and event-driven design
 * - Secure password request and connectivity verification protocols
 * - Integration with ESP-IDF's UART driver, FreeRTOS queues, and MicroUSC system configuration
 *
 * Usage:
 * - Call usc_driver_init() first to configure and install the UART driver.
 * - Use usc_driver_write() for data transmission, usc_driver_request_password() for authentication,
 *   and usc_driver_ping() for connectivity checks.
 * - All functions require prior system and kernel initialization (see init_tiny_kernel()).
 *
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/synced_driver/atomic_sys_op.h"
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Core Configuration --- */

/** Default UART configuration using SDKCONFIG baud rate */
#define STANDARD_UART_CONFIG { \
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE, \
        .data_bits = UART_DATA_8_BITS, \
        .parity    = UART_PARITY_DISABLE, \
        .stop_bits = UART_STOP_BITS_1, \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
        /* .rx_flow_ctrl_thresh = 122, // Only if using HW flow control */ \
    }

/* --- Type Definitions --- */

// Forward declarations
typedef struct usc_driver_t *uscDriverHandler;

// esp_err_t init_usc_bit_manip(usc_bit_manip *bit_manip);

esp_err_t init_configuration_storage(void);

//void usc_driver_clean_data(usc_driver_t *driver);

//uint32_t usc_driver_get_data(const int i);

/**
 * @brief Initialize a UART-based driver for the ESP32.
 * 
 * This function configures and installs a UART driver with the specified parameters,
 * assigns GPIO pins, and registers a data processing callback. It performs validation
 * of the UART configuration, port configuration, and callback function.
 * 
 * @param driver_name     Name of the driver for logging/identification (nullable).
 * @param uart_config     UART configuration (baud rate, data bits, parity, etc.).
 *                            Must be a valid `uart_config_t` struct.
 * @param port_config     UART port configuration (GPIO pins, buffer sizes, etc.).
 *                            Must include valid GPIO assignments and buffer sizes.
 * @param driver_process  Callback function to handle received serial data.
 *                            If NULL, initialization will fail.
 * 
 * @return
 * - ESP_OK: Driver initialized successfully.
 * - ESP_ERR_INVALID_ARG: Invalid `port_config` (e.g., invalid GPIO pins) or `driver_process` is NULL.
 * - ESP_FAIL: Invalid `uart_config` (e.g., unsupported baud rate) or UART driver installation failed.
 * 
 * @note
 * - The caller is responsible for ensuring GPIO pins are valid and not used by other peripherals.
 * - If `driver_name` is NULL, a default name will be used in logs.
 * - The UART driver must be uninstalled with `usc_driver_deinit()` when no longer needed.
 */
esp_err_t usc_driver_init( const char *const driver_name,
                           const uart_config_t uart_config, 
                           const uart_port_config_t port_config, 
                           const usc_data_process_t driver_process);

//void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif