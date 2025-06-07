/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bit_manip.h
 * @brief Bitfield-based driver slot management for the MicroUSC system.
 *
 * This header provides thread-safe utilities for managing driver slot allocation and priority
 * using bitfields, with FreeRTOS critical section protection. It is designed for efficient
 * embedded driver management on ESP32/ESP8266 platforms.
 *
 * The API includes routines for:
 *   - Initializing the driver slot bitfield and concurrency lock
 *   - Finding the first available (unset) driver slot
 *   - Atomically occupying a driver slot for exclusive use
 *
 * Usage and organization:
 *   - Call init_configuration_storage() before using any other functions in this API.
 *   - Use getCurrentEmptyDriverIndex() to find a free slot, or getCurrentEmptyDriverIndexAndOccupy()
 *     to allocate and mark a slot as occupied.
 *   - All functions are safe for use in concurrent FreeRTOS tasks.
 *
 * @note Proper initialization and slot management are essential for safe dynamic driver
 *       registration and task scheduling in MicroUSC-based embedded applications.
 *
 * @author Alejandro Ramirez
 * @date 2025-06-07
 */

#pragma once 

#include "freertos/FreeRTOS.h"
#include "esp_system.h"

#ifdef __cplusplus
extern {
#endif

/**
 * @brief Initializes the configuration storage for driver priorities.
 *
 * Calls the internal bit manipulation initializer for the global priority_storage.
 * This must succeed before proceeding with buffer allocation.
 *
 * @return ESP_OK on success, ESP_ERR_NO_MEM if initialization fails.
 */
esp_err_t init_configuration_storage(void);


/**
 * @brief Finds the first available (unset) driver bit index.
 *
 * Enters a critical section to safely read the current active driver bits.
 * Scans each bit from 0 to DRIVER_MAX-1 and returns the index of the first bit that is 0.
 *
 * @return Index of the first available driver bit, or NOT_FOUND if all are occupied.
 */
UBaseType_t getCurrentEmptyDriverIndex(void);

/**
 * @brief Finds and occupies the first available driver bit index.
 *
 * Calls getCurrentEmptyDriverIndex() to find the first available bit.
 * If found, enters a critical section and sets that bit as occupied.
 * Logs the new bitmask after occupation.
 *
 * @return Index of the occupied driver bit, or NOT_FOUND if none are available.
 */
UBaseType_t getCurrentEmptyDriverIndexAndOccupy(void);

#ifdef __cplusplus
}
#endif