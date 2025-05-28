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
 * configuration settings for all drivers managed by the MicroUSC system[1][2]. It is primarily used
 * for debugging, diagnostics, and verification to ensure that driver parameters and hardware interface
 * settings are correctly initialized and maintained during development or troubleshooting[1][2][4].
 *
 * Usage:
 *   - Include this header in modules that require runtime driver configuration reporting.
 *   - Call usc_print_driver_configurations() during system startup, initialization, or when
 *     diagnosing issues with driver setup and serial communication[1][3][4].
 *
 * @note This function is designed for ESP32/ESP8266 embedded systems and supports robust
 *       system configuration, hardware interface verification, and startup diagnostics[1][2][4].
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
 * parameters are set as intended during development or troubleshooting[1][2][4].
 *
 * @note Use this function to review hardware interface settings, verify initialization, or log
 * configuration details during system startup or runtime[2][4].
 */
void usc_print_driver_configurations(void);

#ifdef __cplusplus
}
#endif