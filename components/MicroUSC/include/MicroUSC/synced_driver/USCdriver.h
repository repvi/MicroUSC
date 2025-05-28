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
 * in the MicroUSC system on ESP32/ESP8266 platforms[5][6][7]. It integrates with FreeRTOS, memory pools, and atomic
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
        // .rx_flow_ctrl_thresh = 122, // Only if using HW flow control
    }

/* --- Type Definitions --- */

/** @brief Callback for driver events */
typedef void (*usc_event_cb_t)(void *);

/** @brief Callback for data processing */
typedef void (*usc_data_process_t)(void *);

// Forward declarations
typedef struct usc_config_t usc_config_t;
typedef struct usc_driver_t usc_driver_t;
typedef struct usc_bit_manip usc_bit_manip;
typedef struct usc_task_manager_t usc_task_manager_t;
typedef usc_task_manager_t *usc_tasks_t;

/** @brief FreeRTOS queues for UART data (indexed by DRIVER_MAX) */
extern QueueHandle_t uart_queue[DRIVER_MAX]; 

esp_err_t init_usc_bit_manip(usc_bit_manip *bit_manip);

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
esp_err_t usc_driver_init( const char *driver_name,
                           const uart_config_t uart_config, 
                           const uart_port_config_t port_config, 
                           usc_data_process_t driver_process);

/**
 * @brief Write data to a USC (MicroUSC) driver using specified configuration.
 *
 * This function transmits a data buffer to the hardware interface defined by the provided
 * usc_config_t driver settings. It handles the details of serial or peripheral communication,
 * ensuring the data is sent according to the driver’s configuration parameters[1][3][5].
 *
 * @param driver_setting Pointer to the USC driver configuration structure (usc_config_t).
 * @param data Pointer to the data buffer to be transmitted.
 * @param len Number of bytes to write from the data buffer.
 * @return
 *   - ESP_OK: Data was written successfully.
 *   - ESP_FAIL or error code: Transmission failed (see implementation for details).
 *
 * @note This function is typically used in ESP32/ESP8266 embedded systems to abstract serial or
 *       peripheral communication, supporting robust driver and system configuration[1][3][4][5][7].
 *       Proper driver initialization must be performed before calling this function[2][4].
 */
esp_err_t usc_driver_write( const usc_config_t *driver_setting, 
                            const char *data, 
                            const size_t len);

/**
 * @brief Request a password from the USC driver system.
 *
 * This function initiates a password request operation using the specified USC driver configuration.
 * It typically triggers the necessary communication or authentication sequence required by the MicroUSC system,
 * ensuring secure access or operation as defined by the driver settings[1][4].
 *
 * @param driver_setting Pointer to the USC driver configuration structure (usc_config_t).
 * @return
 *   - ESP_OK: Password request initiated successfully.
 *   - ESP_FAIL or other error codes: Failure to initiate the password request (see implementation details).
 *
 * @note Proper driver initialization must be completed before calling this function[2][4].
 *       This function is part of the MicroUSC system configuration and secure communication workflow on ESP32[1][5].
 */
esp_err_t usc_driver_request_password(usc_config_t *driver_setting);

/**
 * @brief Send a ping command to the USC driver system.
 *
 * This function performs a ping operation using the specified USC driver configuration to check
 * the communication status and responsiveness of the MicroUSC hardware interface.
 * It is typically used for diagnostics, connectivity verification, or health checks[1].
 *
 * @param driver_setting Pointer to the USC driver configuration structure (usc_config_t).
 * @return
 *   - ESP_OK: Ping successful, device responded.
 *   - ESP_FAIL or other error codes: Ping failed, no response or communication error.
 *
 * @note Ensure the driver is properly initialized before calling this function.
 *       This function supports robust system diagnostics and error handling in embedded ESP32 applications[1].
 */
esp_err_t usc_driver_ping(usc_config_t *driver_setting);

//void uart_port_config_deinit(uart_port_config_t *uart_config);

#ifdef __cplusplus
}
#endif