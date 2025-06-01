/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uscsystemdef.h
 * @brief System status enumeration for MicroUSC on ESP32/ESP8266.
 *
 * This header defines the microusc_status enumeration, representing various operational
 * and diagnostic states for the MicroUSC system. These states are used throughout the
 * system code to control behavior, manage power modes, handle connectivity, and report
<<<<<<< HEAD
 * hardware or driver status[3][4][5].
=======
 * hardware or driver status.
>>>>>>> GP
 *
 * Usage:
 *   - Include this header in modules that require system-level state management.
 *   - Use microusc_status values to represent and switch between system modes,
<<<<<<< HEAD
 *     such as sleep, pause, connectivity, and error states[3][4][5].
 *
 * @note Designed for modular ESP32/ESP8266 projects using conditional code and
 *       hardware interface control[3][4][5].
=======
 *     such as sleep, pause, connectivity, and error states.
 *
 * @note Designed for modular ESP32/ESP8266 projects using conditional code and
 *       hardware interface control.
>>>>>>> GP
 *
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
<<<<<<< HEAD
    USC_SYSTEM_SUCCESS,           ///< Default/idle system state
=======
    USC_SYSTEM_SUCCESS = 0,       ///< Default/idle system state
>>>>>>> GP
    USC_SYSTEM_OFF,               ///< System powered off
    USC_SYSTEM_SLEEP,             ///< System in sleep mode
    USC_SYSTEM_PAUSE,             ///< System paused
    USC_SYSTEM_RESUME,            ///< System resumes
    USC_SYSTEM_WIFI_CONNECT,      ///< WiFi connection in progress
    USC_SYSTEM_BLUETOOTH_CONNECT, ///< Bluetooth connection in progress
    USC_SYSTEM_LED_ON,            ///< LED is turned on
    USC_SYSTEM_LED_OFF,           ///< LED is turned off
    USC_SYSTEM_MEMORY_USAGE,      ///< Query or report memory usage
    USC_SYSTEM_SPECIFICATIONS,    ///< Query system specifications
    USC_SYSTEM_DRIVER_STATUS,     ///< Query driver status
    USC_SYSTEM_ERROR,             ///< System error state
<<<<<<< HEAD
=======
    USC_SYSTEM_PRINT_SUCCUSS
>>>>>>> GP
} microusc_status;

#ifdef __cplusplus
}
#endif