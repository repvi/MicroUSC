/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uscdef.h
 * @brief Core type definitions and macros for MicroUSC driver system on ESP32/ESP8266
 * 
 * Provides fundamental data structures, enums, and iteration macros used throughout
 * the MicroUSC system. Integrates with FreeRTOS for task management and ESP32 UART
 * hardware interfaces[1][3][4][5].
 *
 * Key components:
 * - Iteration macros for safe data buffer traversal (literate_bytes, define_iteration*)
 * - Driver status enumeration (usc_status_t) for state tracking[3][4]
 * - Configuration structures for UART drivers and task management[1][4][5]
 * - FreeRTOS synchronization primitives (SemaphoreHandle_t, TaskHandle_t)
 *
 * Usage:
 * - Include this header in driver implementation files requiring system types
 * - Use INIT_USC_CONFIG_DEFAULT for safe struct initialization
 * - Leverage iteration macros for buffer processing with optional semaphore control[5][9]
 *
 * @note Maintains compatibility with ESP-IDF's component structure and CMake builds[4][5]
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/synced_driver/esp_uart.h"
#include "MicroUSC/synced_driver/atomic_sys_op.h"

/* Forward declarations */
struct usc_stored_config_t;
struct usc_task_manager_t;

/**
 * @name Buffer Iteration Macros
 * @brief Safe data traversal patterns with optional semaphore control[5][9]
 */
///@{
#define literate_bytes(var, type, len) \
    for (type* begin = (var), *end = (var) + (len); begin < end; begin++)

#define define_iteration(var, type, name, len) \
    for (type* (name) = (var), *end = (var) + (len); (name) < end; (name)++)

#define define_iteration_with_semaphore(var, type, name, len) \
    for (type* (name) = (var), *end = (var) + (len); (name) < end; \
        xSemaphoreGive((name)->sync_signal), (name)++) \
    if (xSemaphoreTake((name)->sync_signal, portMAX_DELAY) == pdTRUE) \
        for (bool hasSemaphore = true; hasSemaphore; hasSemaphore = false)
///@}

/* Type aliases */
typedef char driver_name_t[20];  ///< Driver identifier string
typedef char serial_key_t[10];   ///< Security token storage
typedef bool init_safety;        ///< Initialization state flag

/**
 * @name Initialization Constants
 * @brief Safety markers for driver initialization states[4][8]
 */
///@{
#define STATIC_INIT_SAFETY  static init_safety
#define initZERO            false  ///< Uninitialized state
#define initONE             true   ///< Initialized state
#define DRIVER_NAME_SIZE    (sizeof(driver_name_t))  ///< Maximum driver name length
///@}

/**
 * @brief Driver communication and operational states
 * 
 * Used in usc_config_t.status to track UART driver lifecycle[3][4][5]
 */
typedef enum {
    NOT_CONNECTED,         ///< Hardware not detected
    CONNECTED,             ///< Physical layer established
    DISCONNECTED,          ///< Graceful termination
    ERROR,                 ///< Unrecoverable fault
    DATA_RECEIVED,         ///< Successful RX completion
    DATA_SENT,             ///< Successful TX completion
    DATA_SEND_ERROR,       ///< TX failure (checksum/parity)
    DATA_RECEIVE_ERROR,    ///< RX failure (framing/overflow)
    DATA_SEND_TIMEOUT,     ///< TX blocked > SEMAPHORE_WAIT_TIME
    DATA_RECEIVE_TIMEOUT,  ///< RX incomplete by deadline
    DATA_SEND_COMPLETE,    ///< TX confirmed by peer
    DATA_RECEIVE_COMPLETE, ///< RX validated
    TIME_OUT,              ///< General operation timeout
} usc_status_t;

/**
 * @brief UART driver configuration bundle
 * 
 * Contains hardware parameters and data buffers for serial communication[3][6]
 */
struct usc_config_t {
    uart_port_config_t uart_config;  ///< ESP32 UART port/pin assignments
    SerialDataQueueHandler data;     ///< RX/TX buffer management
    driver_name_t driver_name;       ///< Human-readable identifier
    usc_status_t status;             ///< Current state machine position
    uint32_t baud_rate;              ///< Link speed (bits/sec)
    bool has_access;                 ///< Security clearance flag
};

/**
 * @brief FreeRTOS task control structure
 * 
 * Manages driver-related tasks in ESP32 dual-core environment[5][8]
 */
struct usc_task_manager_t {
    TaskHandle_t task_handle;    ///< UART control task (Core 1)
    TaskHandle_t action_handle;  ///< Data processing task (Core 0)
    bool active;                 ///< Task lifecycle flag
};

/**
 * @brief Base driver configuration container
 */
struct usc_driver_base_t {
    struct usc_config_t driver_setting;  ///< Hardware interface parameters
    struct usc_task_manager_t driver_tasks; ///< Task scheduling controls
    UBaseType_t priority;                ///< FreeRTOS task priority[5]
};

/**
 * @brief Complete driver instance definition
 * 
 * Used in linked list management via genList.h utilities[4][9]
 */
struct usc_driver_t {
    struct usc_driver_base_t driver_storage; ///< Configuration and tasks
    UBaseType_t priority;                    ///< Execution precedence
    SemaphoreHandle_t sync_signal;           ///< Thread synchronization
};

/**
 * @brief Default configuration initializer
 * 
 * Usage:
 * struct usc_config_t cfg = INIT_USC_CONFIG_DEFAULT;
 */
#define INIT_USC_CONFIG_DEFAULT { \
    .baud_rate = 0, \
    .data = {0}, \
    .driver_name = "", \
    .has_access = false, \
    .status = NOT_CONNECTED, \
    .uart_config = {0} \
}

#ifdef __cplusplus
}
#endif