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
#include "MicroUSC/uscUniversal.h"
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
typedef void (*usc_process_t)(void *);

typedef struct usc_driver_t *uscDriverHandler;

// esp_err_t init_usc_bit_manip(usc_bit_manip *bit_manip);

// esp_err_t init_configuration_storage(void);

//void usc_driver_clean_data(usc_driver_t *driver);

//uint32_t usc_driver_get_data(const int i);

/**
 * @brief Initialize a UART-based driver for the ESP32.
 * 
 * This function sets up a UART driver with the specified configurations, assigns 
 * GPIO pins, and registers a callback for data processing. It validates the parameters 
 * and ensures correct driver installation.
 * 
 * @param driver_name     Optional identifier for the driver (nullable). If NULL, a default name is used.
 * @param uart_config     UART configuration structure (`uart_config_t`), defining baud rate, data bits, etc.
 * @param port_config     UART port configuration (`uart_port_config_t`), specifying GPIO mappings and buffer sizes.
 * @param driver_process  Callback function (`usc_process_t`) for handling received UART data. **Must not be NULL**.
 * @param stack_size      Stack size for UART driver task (recommended: appropriate for expected data loads).
 * 
 * @return
 * - **ESP_OK**: Successfully initialized the driver.
 * - **ESP_ERR_INVALID_ARG**: Provided `port_config` has invalid GPIO settings or `driver_process` is NULL.
 * - **ESP_FAIL**: Invalid `uart_config` (e.g., unsupported baud rate) or failure in UART driver installation.
 * 
 * @note
 * - The caller must verify that assigned GPIO pins do not conflict with other peripherals.
 * - Ensure correct stack size based on UART traffic for optimal performance.
 * - The driver must be **properly deinitialized** using `usc_driver_deinit()` when no longer needed.
 */
esp_err_t usc_driver_init(const char *const driver_name,
                          const uart_config_t uart_config, 
                          const uart_port_config_t port_config, 
                          const usc_process_t driver_process,
                          const stack_size_t stack_size);

/**
 * @brief Retrieve UART data from the driver.
 * 
 * This function provides a way to fetch received data from a UART-based driver
 * via the provided `driver` handle.
 * 
 * @param driver  Handle to the initialized UART driver (`uscDriverHandler`).
 * 
 * @return
 * - The **data buffer** received via UART.
 * - If the driver is invalid or uninitialized, behavior is **undefined**.
 * 
 * @note
 * - Since the `uscDriverHandler` structure’s internal elements are **inaccessible**, 
 *   this API acts as the correct method for extracting UART data.
 * - Ensure that the driver instance is **initialized and valid** before calling this function.
 */
uint32_t usc_driver_get_data(uscDriverHandler driver);

esp_err_t usc_send_data(uscDriverHandler driver, uint32_t data);

#ifdef __cplusplus
}
#endif