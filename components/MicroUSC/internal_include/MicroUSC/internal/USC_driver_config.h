/*
 * SPDX-FileCopyrightText: 2025 Alejandro Ramirez
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file USC_driver_config.h
 * @brief MicroUSC driver configuration and compiler abstraction layer for ESP32/ESP8266
 * 
 * This header provides system-wide configuration for MicroUSC drivers, including:
 * - Version management
 * - Compiler-specific optimizations and attributes
 * - FreeRTOS task parameters
 * - Serial communication constants
 * - Memory alignment requirements
 * - Safety assertions
 *
 * Key components:
 * 1. Version macros (CURRENT_VERSION_*) for semantic versioning
 * 2. Compiler abstraction layer for GCC/clang attributes (RUN_FIRST, HOT, COLD etc.)
 * 3. Serial protocol constants (SERIAL_KEY, REQUEST_KEY)
 * 4. FreeRTOS task configuration (stack sizes, priorities, core affinity)
 * 5. Architecture-specific alignment requirements
 *
 * @note Designed for ESP-IDF projects using CMake component structure
 * @author Alejandro Ramirez
 * @date May 26, 2025
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* FreeRTOS dependencies */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/semphr.h>
#include "freertos/event_groups.h"
#include "stdbool.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @name Version Configuration
 * @brief Semantic versioning for MicroUSC driver framework
 */
#define CURRENT_VERSION_MAJOR             (0)
#define CURRENT_VERSION_MINOR             (10)
#define CURRENT_VERSION_PATCH             (3)

#define to_string(x)        #x
#define STRINGIFY(x) to_string(x)

#define USC_Version()       to_string(CURRENT_VERSION_MAJOR) "." \
                            to_string(CURRENT_VERSION_MINOR) "." \
                            to_string(CURRENT_VERSION_PATCH)

/**
 * @name Compiler Optimization Attributes
 * @brief GCC/clang-specific code generation controls
 */
#if defined(__GNUC__) || defined(__clang__)
    #define RUN_FIRST      __attribute__((constructor, used, noinline)) // Early initialization
    #define MALLOC         __attribute__((malloc)) // Memory allocator hint
    #define HOT            __attribute__((hot)) // Performance-critical paths
    #define COLD           __attribute__((cold)) // Cold code paths
    #define ALWAYS_INLINE  __attribute__((always_inline))

    #define UNUSED         __attribute__((unused))
    #define DEPRECATED     __attribute__((deprecated))
    #define USED           __attribute__((used))

    #define OPT0         __attribute__((optimize("O0")))
    #define OPT1         __attribute__((optimize("O1"))) 
    #define OPT2         __attribute__((optimize("O2")))
    #define OPT3         __attribute__((optimize("O3")))

    #define ESP32_ALIGNMENT  __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))

    #define __init __attribute__((constructor))
    #define __noreturn __attribute__((noreturn))
    #define __deprecated __attribute__((deprecated))
#else
    /* Fallbacks for non-GCC compilers */
    #define RUN_FIRST
    #define MALLOC 
    #define HOT   
    #define COLD
    #define ALWAYS_INLINE

    #define __init
    #define __noreturn
    #define __deprecated
#endif

/* Constant optimization helper */
#define OPTIMIZE_CONSTANT(x) \
    (__builtin_constant_p(x) ? optimize_for_constant(x) : general_case(x))

/**
 * @name Serial Protocol Constants
 * @brief Security and protocol control values
 */    
#define SERIAL_DATA_SIZE      (126)

/* Memory architecture configuration */
#define ESP32_ARCHITECTURE_ALIGNMENT_VAR ( sizeof( uint32_t ) )

/**
 * @name Range Checking Macros
 * @brief Input validation helpers
 */
#define INSIDE_SCOPE(x, max) (0 <= (x) && (x) < (max))
#define OUTSIDE_SCOPE(x, max) ((x) < 0 || (max) <= (x))
#define developer_input(x) (x)

/* Baud rate safety check */
#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE CONFIG_ESP_CONSOLE_UART_BAUDRATE
#else
    #define CONFIGURED_BAUDRATE (-1)
#endif
ESP_STATIC_ASSERT(CONFIGURED_BAUDRATE != -1, 
                 "CONFIG_ESP_CONSOLE_UART_BAUDRATE undefined!");

/**
 * @name Task Configuration
 * @brief FreeRTOS task parameters
 */
#define TASK_PRIORITY_START      ( ( UBaseType_t ) ( 10 ) )
#define TASK_STACK_SIZE          (4096) // 8192
#define TASK_CORE_READER         (1) // Core 0: wireless, Core 1: app logic
#define TASK_CORE_ACTION         (0)

/* Timing constants */
#define DELAY_MILISECOND_50       pdMS_TO_TICKS(50)
#define SERIAL_REQUEST_DELAY_MS   (30)
#define SERIAL_KEY_RETRY_DELAY_MS (50)
#define LOOP_DELAY_MS             pdMS_TO_TICKS(10)
#define SEMAPHORE_DELAY           pdMS_TO_TICKS(3)
#define SEMAPHORE_WAIT_TIME       pdMS_TO_TICKS(5000)

#define NOT_FOUND (( uint32_t ) ( -1 ) )

#ifdef __cplusplus
}
#endif