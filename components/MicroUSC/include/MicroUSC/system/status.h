/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file status.h
 * @brief Driver configuration status reporting for MicroUSC on ESP32/ESP8266.
 *
 * This header declares the usc_print_driver_configurations() function, which outputs the current
 * configuration settings for all drivers managed by the MicroUSC system. It is primarily used
 * for debugging, diagnostics, and verification to ensure that driver parameters and hardware interface
 * settings are correctly initialized and maintained during development or troubleshooting.
 *
 * Usage:
 *   - Include this header in modules that require runtime driver configuration reporting.
 *   - Call usc_print_driver_configurations() during system startup, initialization, or when
 *     diagnosing issues with driver setup and serial communication.
 *
 * @note This function is designed for ESP32/ESP8266 embedded systems and supports robust
 *       system configuration, hardware interface verification, and startup diagnostics.
 *
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Print the current driver configurations to the console.
 *
 * This function outputs the active configuration settings for the drivers managed by the system.
 * It is typically used for debugging, diagnostics, or verification purposes to ensure that all driver
 * parameters are set as intended during development or troubleshooting.
 *
 * @note Use this function to review hardware interface settings, verify initialization, or log
 * configuration details during system startup or runtime.
 */
void usc_print_driver_configurations(void);


/**
 * @brief Print ESP32 chip information to console
 *
 * Displays:
 * - Chip model (ESP32 or Other)
 * - Number of CPU cores
 * - Available features (WiFi/BT/BLE)
 */
void print_system_info(void);

/**
 * @brief Display ESP32 memory usage statistics for DMA and internal memory regions
 * 
 * This function queries the ESP-IDF heap capabilities API to retrieve and display
 * memory usage information for DMA-capable and internal memory regions.
 * 
 * @warning If this function crashes with LoadProhibited exception, it indicates
 *          heap corruption has already occurred earlier in program execution.
 *          Common causes include:
 *          - Buffer overflows writing past allocated memory boundaries
 *          - Use-after-free operations accessing freed memory
 *          - Double-free operations corrupting heap metadata
 *          - Memory alignment issues causing improper memory access
 *          - Stack overflow damaging heap structures
 * 
 * @note Function uses local macro MEMORY_TAG for consistent logging output
 * @note All memory values are retrieved as const to prevent modification
 */
void show_memory_usage(void);

#ifdef __cplusplus
}
#endif