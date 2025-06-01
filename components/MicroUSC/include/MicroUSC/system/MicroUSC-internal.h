/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file MicroUSC-internal.h
 * @brief Internal system and power management API for MicroUSC on ESP32/ESP8266.
 *
 * This header provides internal function declarations for system-level configuration,
 * error handling, power management, and RTC variable storage within the MicroUSC library.
 * It enables precise control over sleep/wakeup sources, custom error handler registration,
 * and persistent variable management, supporting robust embedded system design and modular
 * code organization.
 *
 * Key features:
 *   - Sleep mode and wakeup configuration (timer and GPIO-based)
 *   - RTC memory variable storage and retrieval for low-power retention
 *   - Custom and default system error handler management
 *   - System status code management for state transitions and diagnostics
 *   - One-time system setup and initialization routines 
 *   - **Single initialization:** Must be called exactly once at the start of app_main().
 *   - **Priority inheritance:** Sets up FreeRTOS task priorities for driver operations.
 *   - **Resource tracking:** Monitors memory and IRQ usage for embedded constraints.
 *   - **Dependency chain:** Initializes all core subsystems required by MicroUSC.
 *
 * Usage:
 *   - Include this header only in modules requiring direct access to MicroUSC system internals.
 *   - Call initialization and configuration routines during system startup or power mode transitions.
 *   - Use error handler and status code functions for robust error and state management.
 * 
 * Usage Protocol:
 * 1. Call `init_tiny_kernel();` as the very first line in your app_main().
 * 2. Do not re-initialize or call again—subsequent calls will cause resource conflicts.
 * 3. After initialization, all driver, memory, and task management functions may be safely used.
 *
 * 
 * @warning Calling this after any other MicroUSC initialization or driver/memory setup
 *          will cause resource conflicts, memory corruption, and unpredictable system behavior.
 * 
 * @note This header is intended for internal use within the MicroUSC library and should not be exposed
 *       to application-level code unless advanced system control is required.
 *
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#include "MicroUSC/system/uscsystemdef.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*microusc_error_handler)(void *);

/**
 * @brief Set the timer duration for sleep mode wakeup.
 *
 * This function configures the timer duration (in microseconds) for the timer-based wakeup source.
 * When the timer expires, it triggers the ISR used to wake up the ESP32 from sleep mode.
 *
 * Typical usage: Call this function after enabling the timer wakeup source to specify the wakeup interval.
 *
 * @param time The timer duration in microseconds until the wakeup ISR is triggered.
 *
 * @note This is essential for applications requiring precise timed wakeups and low-power operation on ESP32.
 */
void microusc_set_sleep_mode_timer_wakeup(uint64_t time);

/**
 * @brief Enable or disable the timer as a sleep mode wakeup source.
 *
 * This function enables or disables the use of a timer to wake up the ESP32 from sleep mode.
 * When enabled, the timer can be configured to trigger an interrupt service routine (ISR) that wakes the device.
 *
 * Typical usage: Call this function before entering sleep mode to specify whether the timer should be used as a wakeup source.
 *
 * @param option Set to true to enable the timer wakeup, or false to disable it.
 *
 * @note This function is typically used in embedded ESP32 projects where precise timing and wakeup control are required.
 */
void microusc_set_sleep_mode_timer(bool option);

/**
 * @brief Configure a GPIO pin as the wakeup source for ISR.
 *
 * This function sets the specified GPIO pin to be used as the interrupt source for waking up the ESP32 from sleep mode.
 * It configures the pin to trigger an ISR (Interrupt Service Routine) that will wake the device.
 *
 * Typical usage involves calling this function before entering sleep mode to ensure the device can be awakened by the specified GPIO.
 *
 * @param pin The GPIO number to be configured as the wakeup interrupt source.
 *
 * @note The actual interrupt configuration and enabling must be handled separately.
 */
void microusc_set_wakeup_pin(gpio_num_t pin);

/**
 * @brief Enable or disable the GPIO wakeup interrupt pin.
 *
 * This function controls the status of the GPIO interrupt pin used for waking up the ESP32 from sleep mode.
 * When called with 'option' set to true, the wakeup pin is enabled; when set to false, the wakeup pin is disabled.
 * Disabling the pin prevents the associated interrupt from triggering a wakeup event.
 *
 * This function is typically used in conjunction with microusc_set_sleepmode_wakeup_default() to manage
 * the device's wakeup sources and power management behavior.
 *
 * @param option Set to true to enable the GPIO wakeup pin, or false to disable it.
 *
 * @note Disabling the wakeup pin will prevent the device from waking up via the corresponding GPIO interrupt.
 */
void microusc_set_wakeup_pin_status(bool option);

/**
 * @brief Configure the default wakeup function for sleep mode.
 *
 * This function sets up a void pointer to a default wakeup handler
 * that is used as an ISR (Interrupt Service Routine) to wake up
 * the ESP32 from sleep mode. It ensures that, when the microcontroller
 * is in a low-power state, the configured interrupt can properly
 * trigger the wakeup sequence using the assigned handler.
 *
 * Typical use case: Call this function during system initialization
 * or before entering sleep mode to ensure the device can be correctly
 * awakened by external events or interrupts.
 *
 * @note The actual wakeup source and interrupt configuration must be
 *       set up separately, depending on the hardware and application needs.
 */
void microusc_set_sleepmode_wakeup_default(void);

/**
 * @brief Save a system variable to RTC memory.
 *
 * This function attempts to save a variable to RTC (Real-Time Clock) memory, identified by a key.
 * It performs argument validation and will return early without performing any operation if:
 *   - `var` is NULL,
 *   - `size` is 0,
 *   - `key` is 0 or the null character (`'\0'`).
 *
 * @param var   Pointer to the variable data to be saved. Must not be NULL.
 * @param size  Size in bytes of the variable to be saved. Must be greater than 0.
 * @param key   Unique identifier for the variable in RTC memory. Must not be 0 or '\0'.
 *
 * @note
 * - If any argument is invalid, the function returns immediately and does not attempt to save the variable.
 * - It is the caller's responsibility to ensure that `var` points to valid memory of at least `size` bytes.
 *
 * @return None.
 */
void save_system_rtc_var(void *var, const size_t size, const char key);

/**
 * @brief Retrieve a system variable from RTC memory by key.
 *
 * This function searches for a variable previously saved to RTC (Real-Time Clock) memory
 * using the provided key. If the key matches an existing entry, a pointer to the variable's
 * data is returned. If the key does not match any stored entry, or if the key is invalid,
 * the function returns NULL.
 *
 * @param key  Unique identifier for the variable in RTC memory.
 *             Must match one of the keys used in save_system_rtc_var().
 *
 * @return
 *   - Pointer to the variable data if the key is found.
 *   - NULL if the key does not exist in the RTC variable store or is invalid.
 *
 * @note
 * - The returned pointer is to the internal RTC memory storage; do not free or modify it directly.
 * - The key must be a valid identifier previously used to save a variable; otherwise, NULL is returned.
 * - It is the caller's responsibility to ensure the key is valid and to handle a NULL return appropriately.
 */
void *get_system_rtc_var(const char key);

/**
 * @brief Set a custom system error handler for the microcontroller system.
 *
 * This function allows the application to register a custom error handler function
 * for the microcontroller system. The provided handler will be called whenever a
 * system-level error or exception occurs, replacing any previously set handler,
 * including the default handler.
 *
 * Custom error handlers are useful for:
 *   - Logging or reporting errors in an application-specific way
 *   - Attempting recovery actions or cleanup after faults
 *   - Integrating with higher-level error management frameworks
 *   - Providing user feedback or diagnostics
 *
 * The handler function must match the signature defined by `microusc_error_handler`.
 * Passing a NULL pointer may disable error handling or restore default behavior,
 * depending on the system implementation.
 *
 * @param handler  Pointer to the custom error handler function to be registered.
 *
 * @note
 * - The error handler will be invoked in the context where the error occurs, which
 *   may be an interrupt, exception, or main application context.
 * - The handler should be efficient and avoid blocking operations, especially if
 *   called from an interrupt or fault context.
 * - Registering a new handler overrides any previously set handler.
 * - To restore the default error handler, use set_microusc_system_error_handler_default().
 */
void set_microusc_system_error_handler(microusc_error_handler handler, void *var, int size);

/**
 * @brief Set the current system status code for the microUSC subsystem.
 *
 * This function updates the internal status of the microUSC system by assigning
 * a new status code. It is intended for use in low-level operations within the system
 * task or related system management code, allowing the system to transition between
 * defined operational states (such as default, error, or custom states).
 *
 * Typical use cases include:
 *   - Indicating successful initialization or startup
 *   - Signaling error or fault conditions
 *   - Transitioning between operational modes (e.g., standby, active, recovery)
 *
 * @param code The status code to set, defined by the microusc_status enumeration.
 *
 * @note This function should be called only from trusted, low-level system code to
 *       maintain system integrity. The status code should be a valid value from the
 *       microusc_status enum or macro set.
 *
 * @example
 *   set_microusc_system_code(USC_SYSTEM_SUCCESS);
 *   set_microusc_system_code(USC_SYSTEM_ERROR);
 */
void set_microusc_system_code(microusc_status code);

/**
 * @brief Set the default system error handler for the microcontroller.
 *
 * This function installs or restores the default error handler for the system.
 * The default error handler is responsible for catching, reporting, and managing
 * system-level errors or exceptions that occur during program execution.
 *
 * Typical responsibilities of the default error handler may include:
 *   - Logging or displaying error messages for debugging or user feedback.
 *   - Recording error codes or statuses for later retrieval or analysis.
 *   - Attempting safe recovery from certain error conditions, if possible.
 *   - Halting the system or triggering a system reset in case of fatal errors.
 *
 * This function is intended to be called during system initialization or when
 * resetting the error handling mechanism to a known-safe state.
 *
 * @note
 * - Custom error handlers may be installed elsewhere in the code. Calling this
 *   function will override any previously set custom handler with the default.
 * - The implementation may vary depending on the system architecture and requirements.
 * - For embedded systems, the default handler should be robust and avoid undefined behavior.
 */
void set_microusc_system_error_handler_default(void);

/**
 * @brief Set the MicroUSC system status code and trigger corresponding system behavior.
 *
 * This function updates the status of the MicroUSC system by accepting a value of type `microusc_status`.
 * Based on the value of `code` passed, the ESP32 will execute specific actions or state transitions
 * defined for your MicroUSC system. This may involve changing system modes, updating indicators,
 * triggering hardware actions, or altering task behavior.
 *
 * @param code  The status code to set for the MicroUSC system. The value should be a member of the
 *              `microusc_status` enum, representing the desired system state or command.
 *
 * @note
 * - The mapping between `microusc_status` values and system actions must be documented elsewhere (e.g., in the enum definition or system design documentation).
 * - This function is intended for use in embedded ESP32 applications and should be called whenever a system state change is required.
 * - Ensure thread safety if this function is called from multiple tasks or ISRs.
 */
void set_microusc_system_code(microusc_status code);


void microusc_infloop(void);

/**
 * @brief Initialize the MicroUSC system components and resources.
 *
 * This function performs one-time initialization of critical system components
 * required for MicroUSC operation, including hardware interfaces and internal
 * data structures. It must be called exactly once during system startup.
 *
 * @return 
 * - ESP_OK: Initialization successful
 * - Other error codes: Initialization failed (specific error depends on implementation)
 *
 * @note Calling this function multiple times may cause resource leaks, 
 *       hardware conflicts, or undefined behavior due to duplicate initialization.
 *       Ensure it is only called once during the application lifecycle, typically
 *       in app_main() before entering the main execution loop.
 */
esp_err_t microusc_system_setup(void);


/**
 * @brief Initialize the Tiny Kernel core for the MicroUSC library.
 *
 * This function must be called first during system startup on the ESP32 before using any other MicroUSC library features.
 * It sets up the core kernel structures and services essential for the MicroUSC system to function correctly.
 *
 * @note RUN_FIRST: This function should be invoked at the very start of your application, typically at the beginning of app_main().
 *       Only call this function once during the system's lifetime.
 */
void init_MicroUSC_system(void);

#ifdef __cplusplus
}
#endif