#pragma once

#include "USC_driver_config.h" // should rename

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

esp_err_t microusc_system_task(void);

void set_microusc_system_code(microusc_status code);

#ifdef __cplusplus
}
#endif