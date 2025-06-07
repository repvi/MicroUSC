/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system_attr.h
 * @brief Built-in LED control and system status indication for MicroUSC on ESP32/ESP8266.
 *
 * This header provides functions for initializing and controlling the onboard (built-in) LED,
 * as well as mapping MicroUSC system status codes to LED behavior. It is intended for
 * quick hardware diagnostics, system feedback, and status indication in embedded ESP32 type projects.
 *
 * Features:
 * - Initialization of the onboard LED hardware interface
 * - Direct control of LED state (on/off) for application feedback
 * - System status-driven LED behavior for diagnostics and state indication
 *
 * Usage:
 * 1. Call `init_builtin_led()` during system startup to configure the LED pin.
 * 2. Use `builtin_led_set(true/false)` to turn the LED on or off as needed.
 * 3. Use `builtin_led_system(status)` to automatically reflect MicroUSC system status via LED patterns or states.
 *
 * @note Designed for modular MicroUSC system code and ESP32/ESP8266 hardware abstraction.
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#include "MicroUSC/system/uscsystemdef.h"
#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the onboard (built-in) LED hardware interface.
 *
 * Configures the GPIO pin connected to the onboard LED for output mode.
 * Should be called once during system initialization.
 */
extern void init_builtin_led(void);

/**
 * @brief Set the state of the onboard LED.
 *
 * @param state true to turn the LED on, false to turn it off.
 *
 * Provides direct control of the onboard LED for application-level feedback or debugging.
 */
void builtin_led_set(bool state);

/**
 * @brief Update the onboard LED based on MicroUSC system status.
 *
 * @param status MicroUSC system status code (microusc_status).
 *
 * Maps system status codes to LED behavior (e.g., blink patterns or solid on/off) for
 * real-time diagnostics and system feedback in embedded applications.
 */
void builtin_led_system(microusc_status status);

#ifdef __cplusplus
}
#endif