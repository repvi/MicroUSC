/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file manager.h
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

//#include "MicroUSC/wireless/wifi.h"
//#include "MicroUSC/wireless/mqtt.h"
#include "MicroUSC/system/uscsystemdef.h"
#include "MicroUSC/system/rtc.h"
#include "MicroUSC/system/sleep.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*microusc_error_handler)(void *);

/**
 * @brief Configure a GPIO pin as an interrupt source for the MicroUSC system and register an ISR.
 *
 * This function determines the GPIO pin from the provided gpio_config_t's pin_bit_mask,
 * removes any existing ISR handler for that pin, configures the pin with the given settings,
 * and attaches the specified ISR handler. It also sets the status code that will be triggered
 * when the ISR is called.
 *
 * Typical usage: Call this during system initialization to set up a wakeup or event pin.
 *
 * @param io_config      GPIO configuration structure specifying the pin and settings.
 * @param trigger_status The MicroUSC status code to associate with this ISR event.
 */
void microusc_system_isr_pin(gpio_config_t io_config, microusc_status trigger_status);

// void microusc_start_wifi(char *const ssid, char *const password);

/**
 * @brief Performs a complete system restart of the ESP32
 *
 * This function triggers a hardware reset of the ESP32. The noreturn attribute
 * indicates to the compiler that this function will never return to the caller.
 *
 * @note This function will immediately restart the system without any cleanup
 * @warning Any unsaved data will be lost
 */
__attribute__((noreturn)) void microusc_system_restart(void);

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
 * @brief Starts the MQTT service with the specified broker URL and buffers
 *
 * @param url         MQTT broker URL string
 * @param buffer_size Size for receive buffer
 * @param out_size   Size for publish buffer
 *
 * @return ESP_OK if successful, error code otherwise
 * 
 * @note Requires active WiFi connection
 */
// MqttMaintainerHandler microusc_system_start_mqtt_service(esp_mqtt_client_config_t *mqtt_cfg);

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
 *   send_microusc_system_status(USC_SYSTEM_SUCCESS);
 *   send_microusc_system_status(USC_SYSTEM_ERROR);
 */
void send_microusc_system_status(microusc_status code);

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
void send_microusc_system_status(microusc_status code);

/**
 * @brief Infinite loop function for the MicroUSC system.
 *
 * This function is intended to be called when the system enters an unrecoverable state
 * or when a critical error occurs that prevents normal operation. It will enter an infinite
 * loop, effectively halting the system's execution.
 *
 * This is typically used in error handling routines to prevent further execution of code
 * that may lead to undefined behavior or additional errors.
 *
 * @note This function should not return; it will block indefinitely.
 */
__attribute__((noreturn)) void microusc_infloop(void);

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