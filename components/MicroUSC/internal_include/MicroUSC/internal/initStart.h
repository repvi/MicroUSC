/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file initState.h
 * @brief Driver and task initialization state management for the MicroUSC system on ESP32/ESP8266.
 *
 * This header provides core functions and definitions for initializing, configuring, and managing
 * the state of driver structures and associated tasks within the MicroUSC library. It is designed
 * for robust embedded system startup, dynamic driver registration, task scheduling, and safe 
 * hardware interface management on ESP32/ESP8266 platforms.
 *
 * The API includes routines for:
 *   - Setting default driver and task configurations
 *   - Adding, removing, and freeing driver instances from system-managed lists
 *   - Managing task state, handlers, and default parameters
 *   - Deactivating drivers and ensuring safe shutdown or reconfiguration
 *
 * Organization and usage:
 *   - Functions are intended to be called during system initialization or reconfiguration,
 *     typically at startup or when dynamically managing drivers in a FreeRTOS environment.
 *   - The file is part of a modular component structure, supporting clear separation of concerns
 *     and maintainable C code organization using CMake and ESP-IDF build systems.
 *
 * @note Proper use of these routines is essential for preventing undefined behavior,
 *       memory leaks, or hardware conflicts in embedded applications.
 *       Manual file management and explicit initialization are recommended for clarity and reliability.
 *
 * @author Alejandro Ramirez
 * @date 5/26/2026
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "MicroUSC/system/memory_pool.h"
#include "MicroUSC/internal/USC_driver_config.h"
#include "MicroUSC/internal/genList.h"
#include "MicroUSC/internal/uscdef.h"
#include "esp_err.h"

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif

struct usc_driverList {
    struct usc_driver_t driver;
    struct list_head list;
};
 
 struct usc_driversHandler {
     struct usc_driverList driver_list;
     size_t size;
     size_t max; // need to initial with size already, do not change size
     SemaphoreHandle_t lock;
};

extern struct usc_driversHandler driver_system;

extern memory_block_handle_t mem_block_driver_nodes;

/**
 * @brief Add a driver to the MicroUSC system with a specified FreeRTOS priority.
 *
 * Inserts a driver into the system's linked list of active drivers (using list_add_tail()),
 * assigning its execution priority for task scheduling. Ensures thread-safe access
 * to the driver list in FreeRTOS-based ESP32/ESP8266 projects.
 *
 * @param driver   Pre-initialized usc_driver_t structure (UART, etc.)
 * @param priority FreeRTOS task priority (0 <= priority <= configMAX_PRIORITIES-1)
 *
 * @note Duplicate driver additions are not checked; ensure uniqueness to avoid conflicts.
 */
void addSingleDriver(const struct usc_driver_t *driver, const UBaseType_t priority);

/**
 * @brief Remove a single driver from the MicroUSC system.
 *
 * Safely unlinks the driver from the active list (using list_del()) and frees
 * associated resources. Designed for use with ESP32 dynamic driver configurations.
 *
 * @param item Pointer to the usc_driverList node containing the driver
 *
 * @warning Does not deinitialize hardware interfaces - call driver-specific deinit first.
 */
void removeSingleDriver(struct usc_driverList *item);

/**
 * @brief Clear all drivers from the MicroUSC system.
 *
 * Iterates through the driver list (using list_for_each_safe()), removes all entries,
 * and frees all allocated memory. Critical for preventing leaks during system shutdown
 * or reconfiguration in embedded applications.
 */
void freeDriverList(void);

/**
 * @brief Check if a MicroUSC system task is currently active.
 *
 * @param task Pointer to usc_task_manager_t structure
 * @return true - Task is in RUNNING or READY state
 * @return false - Task is SUSPENDED or DELETED
 *
 * @note Uses FreeRTOS internal task states. Safe for ISR context with proper synchronization.
 */
bool getTask_status(const struct usc_task_manager_t *task);

/**
 * @brief Control task execution state.
 *
 * @param task   Pointer to usc_task_manager_t
 * @param active true=vTaskResume(), false=vTaskSuspend()
 *
 * @note Suspend/resume operations are atomic. Does not handle task deletion.
 */
void setTask_status(struct usc_task_manager_t *task, bool active);

/**
 * @brief Reset task notification and event handlers.
 *
 * Sets task->notification_handler and task->event_handler to NULL.
 * Essential for safe task recycling in dynamic ESP32 applications.
 */
void setTaskHandlersNULL(struct usc_task_manager_t *task);

/**
 * @brief Reset task to default configuration.
 *
 * Sets:
 * - Priority to tskIDLE_PRIORITY
 * - Stack depth to USC_TASK_STACK_DEFAULT
 * - Handlers to NULL
 * - State to SUSPENDED
 *
 * Prepares tasks for reuse while avoiding memory reallocation.
 */
void setTaskDefault(struct usc_task_manager_t *task);

#define WAIT_FOR_RESPONSE            ( pdMS_TO_TICKS( 1000 ) )

esp_err_t init_driver_list_memory_pool(void);

esp_err_t init_hidden_driver_lists(void);

/**
 * @brief Initialize a driver structure with default settings.
 *
 * This function sets all fields of the specified usc_driver_t structure to their default values,
 * ensuring the driver is in a known, safe state before further configuration or use.
 * It should be called once during driver setup, typically at system startup or before
 * assigning custom parameters to the driver.
 *
 * @param driver Pointer to the usc_driver_t structure to initialize.
 * @return ESP_OK if initialization was successful, or an error code on failure.
 *
 * @note Proper default initialization helps prevent undefined behavior and supports robust
 * system configuration in ESP32-based and embedded MicroUSC projects.
 */
esp_err_t set_driver_default(struct usc_driver_t *driver);

/**
 * @brief Initialize the default task configuration for a driver.
 *
 * This function sets the task-related fields of the specified usc_driver_t structure
 * to their default values, including priority, stack size, handlers, and state.
 * It is typically called after set_driver_default() to ensure both driver and task
 * parameters are correctly initialized before task creation or scheduling.
 *
 * @param driver Pointer to the usc_driver_t structure whose task configuration will be initialized.
 * @return ESP_OK if task defaults were set successfully, or an error code on failure.
 *
 * @note This function is essential for safe task management and predictable scheduling
 * in FreeRTOS-based ESP32/ESP8266 applications using the MicroUSC library.
 */
esp_err_t set_driver_default_task(struct usc_driver_t *driver);

/**
 * @brief Deactivate a driver and disconnect all associated functions.
 *
 * This function marks the specified usc_driver_t structure as inactive, disabling its operation
 * and disconnecting any functions or handlers linked to the driver.
 * It is typically used during shutdown, reconfiguration, or error handling to ensure
 * the driver does not participate in further system activity.
 *
 * @param driver Pointer to the usc_driver_t structure to deactivate.
 *
 * @note After calling this function, the driver should not be used until it is re-initialized.
 *       This supports safe and robust hardware interface management in ESP32 and embedded systems.
 */
void set_driver_inactive(struct usc_driver_t *driver);

#ifdef __cplusplus
}
#endif