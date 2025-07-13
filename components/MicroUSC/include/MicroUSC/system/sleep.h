#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#define CONVERT_TO_SLEEPMODE_TIME(x) ( uint64_t ) ( pdMS_TO_TICKS(x) * portTICK_PERIOD_MS * 1000ULL )
#define DEFAULT_LIGHTMODE_TIME CONVERT_TO_SLEEPMODE_TIME(5000) // 5 seconds

/**
 * @brief Set timer duration for sleep mode wakeup in microseconds
 * @param time Duration in microseconds until wakeup
 */
void sleep_mode_timer_wakeup(uint64_t time);

/**
 * @brief Enable/disable timer as sleep mode wakeup source
 * @param option True to enable, false to disable timer wakeup
 */
void sleep_mode_timer(bool option);

/**
 * @brief Set GPIO pin as wakeup source
 * @param pin GPIO number for wakeup interrupt
 */
void sleep_mode_wakeup_pin(gpio_num_t pin);

/**
 * @brief Enable/disable GPIO wakeup interrupt pin
 * @param option True to enable, false to disable wakeup pin
 */
void sleep_mode_wakeup_pin_status(bool option);

/**
 * @brief Configure default wakeup handler for sleep mode
 * Sets up ISR handler for waking ESP32 from sleep mode
 */
void sleep_mode_wakeup_default(void);

/**
 * @brief Enter deep sleep mode if wakeup sources are configured
 * 
 * Enters deep sleep if either timer or GPIO wakeup is enabled.
 * Wakes up on:
 * - Timer expiration (if enabled)
 * - GPIO pin trigger (if enabled)
 * 
 * @note Program restarts after wakeup
 */
void sleep_mode(void);

#ifdef __cplusplus
}
#endif