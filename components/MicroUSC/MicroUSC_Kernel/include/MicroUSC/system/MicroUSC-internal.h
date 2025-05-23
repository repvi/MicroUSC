#pragma once

#include "MicroUSC/internal/USC_driver_config.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*microusc_error_handler)();

typedef enum {
    USC_SYSTEM_DEFAULT,
    USC_SYSTEM_OFF,
    USC_SYSTEM_SLEEP,
    USC_SYSTEM_PAUSE,
    USC_SYSTEM_WIFI_CONNECT,
    USC_SYSTEM_BLUETOOTH_CONNECT,
    USC_SYSTEM_LED_ON,
    USC_SYSTEM_LED_OFF,
    USC_SYSTEM_MEMORY_USAGE,
    USC_SYSTEM_SPECIFICATIONS,
    USC_SYSTEM_DRIVER_STATUS,
    USC_SYSTEM_ERROR,
} microusc_status;

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
void set_microusc_system_error_handler(microusc_error_handler handler);

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
 *   set_microusc_system_code(USC_SYSTEM_DEFAULT);
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
 * - This function is intended for use in embedded ESP32 applications and should be called whenever a system state change is required[1][4].
 * - Ensure thread safety if this function is called from multiple tasks or ISRs.
 */
void set_microusc_system_code(microusc_status code);

esp_err_t microusc_system_task(void);

#ifdef __cplusplus
}
#endif