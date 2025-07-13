#pragma once

#include  "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif