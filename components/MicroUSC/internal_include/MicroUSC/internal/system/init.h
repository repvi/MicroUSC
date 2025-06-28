#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

/**
 * @brief Update RTC cycle counter and validate checksum
 * 
 * Checks counter validity using checksum. Resets counter on corruption,
 * otherwise increments it. Updates checksum and logs failures.
 */
void set_rtc_cycle(void);

/**
 * @brief Increment RTC cycle counter
 * Wrapper for set_rtc_cycle()
 */
void increment_rtc_cycle(void);

/**
 * @brief Initialize complete system memory space
 * 
 * Sets up:
 * - Memory handlers and mutexes
 * - Configuration storage
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t init_system_memory_space(void);

#ifdef __cplusplus
}
#endif