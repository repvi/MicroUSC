/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tiny_kernel.h
 * @brief MicroUSC Tiny Kernel Initialization for ESP32/ESP8266 Embedded Systems
 *
 * This header defines the foundational initialization function for the MicroUSC library's core kernel.
 * The Tiny Kernel provides essential services for driver management, memory allocation, task scheduling,
 * and system coordination. All MicroUSC components depend on proper kernel initialization to function.
 *
 * --------------------------------------------------------------------------------
 * CRITICAL: All MicroUSC system functions—such as driver management, memory pools,
 * task scheduling, and hardware abstraction—**WILL NOT WORK** unless init_tiny_kernel()
 * is called first during system startup. Failure to do so will result in undefined
 * behavior, memory corruption, driver deadlocks, and hardware faults.
 * --------------------------------------------------------------------------------
 *
 * Key Features:
 * - **Single initialization:** Must be called exactly once at the start of app_main().
 * - **Priority inheritance:** Sets up FreeRTOS task priorities for driver operations.
 * - **Resource tracking:** Monitors memory and IRQ usage for embedded constraints.
 * - **Dependency chain:** Initializes all core subsystems required by MicroUSC.
 *
 * Usage Protocol:
 * 1. Call `init_tiny_kernel();` as the very first line in your app_main().
 * 2. Do not re-initialize or call again—subsequent calls will cause resource conflicts.
 * 3. After initialization, all driver, memory, and task management functions may be safely used.
 *
 * Example:
 * ```
 * void app_main() {
 *     init_tiny_kernel(); // MUST BE FIRST
 *     // ... other MicroUSC initialization ...
 * }
 * ```
 *
 * @warning Calling this after any other MicroUSC initialization or driver/memory setup
 *          will cause resource conflicts, memory corruption, and unpredictable system behavior.
 *
 * @note This header is part of the MicroUSC system codebase for robust ESP32 and ESP32s3 development.
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Tiny Kernel core for the MicroUSC library.
 *
 * This function must be called first during system startup on the ESP32 before using any other MicroUSC library features.
 * It sets up the core kernel structures and services essential for the MicroUSC system to function correctly.
 *
 * @note RUN_FIRST: This function should be invoked at the very start of your application, typically at the beginning of app_main().
 *       Only call this function once during the system's lifetime.
 */
void init_tiny_kernel(void); // RUN_FIRST

#ifdef __cplusplus
}
#endif