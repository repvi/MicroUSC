#ifndef __USC_DRIVER_CONFIG_H
#define __USC_DRIVER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/semphr.h>
#include "freertos/event_groups.h"
#include <stdint.h>
#include <stddef.h>

#define CURRENT_VERSION_MAJOR             (0)
#define CURRENT_VERSION_MINOR             (10)
#define CURRENT_VERSION_PATCH             (3)

#define to_string(x)        #x
#define USC_Version()       to_string(CURRENT_VERSION_MAJOR) "." \
                            to_string(CURRENT_VERSION_MINOR) "." \
                            to_string(CURRENT_VERSION_PATCH)

#if defined(__GNUC__) || defined(__clang__)
    #define RUN_FIRST      __attribute__((constructor, used, noinline)) // probably might not use
    #define MALLOC         __attribute__((malloc)) // used for dynamic memory functions
    #define HOT            __attribute__((hot)) // for critical operations (Need most optimization)
    #define COLD           __attribute__((cold)) // not much much. Use less memory but slower execution of function. Used like in initializing
    
    #define UNUSED         __attribute__((unused))
    #define DEPRECATED     __attribute__((deprecated))
    #define USED           __attribute__((used))

    #define OPT0         __attribute__((optimize("O0")))
    #define OPT1         __attribute__((optimize("O1")))
    #define OPT2         __attribute__((optimize("O2")))
    #define OPT3         __attribute__((optimize("O3")))

    #define ESP32_ALIGNMENT  __attribute__((aligned(ESP32_ARCHITECTURE_ALIGNMENT_VAR)))
#else
    #define RUN_FIRST
    #define MALLOC 
    #define HOT   
    #define COLD

    #define UNUSED
    #define DEPRECATED
    #define USED

    #define OPT0 
    #define OPT1 
    #define OPT2 
    #define OPT3

    #define ESP32_ALIGNMENT
#endif

#ifdef OPTIMIZE_CONSTANT
    #undef OPTIMIZE_CONSTANT
#endif
#define OPTIMIZE_CONSTANT(x) \
    (__builtin_constant_p(x) ? optimize_for_constant(x) : general_case(x))
    
#define DRIVER_MAX              (2)

#define SERIAL_KEY      "123456789" // Password for the program
#define REQUEST_KEY     "GSKx" // Used for requesting the passcode for the driver
#define PING            "ping" // Used for making sure there is a connection
#define SERIAL_REQUEST_SIZE  ( sizeof( uint32_t ) ) 

#define SERIAL_DATA_SIZE      (126)

#define ESP32_ARCHITECTURE_ALIGNMENT_VAR ( sizeof( uint32_t ) ) // used for the ESP32 architecture
    
#define INSIDE_SCOPE(x, max) (0 <= (x) && (x) < (max))
#define OUTSIDE_SCOPE(x, max) ((x) < 0 || (max) <= (x))
#define developer_input(x) (x)

// DRAM_ATTR // put in IRAM, not in flash, not in PSRAM

// needs baud rate implementation
#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       CONFIG_ESP_CONSOLE_UART_BAUDRATE

#endif
#ifndef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       (-1)

#endif

ESP_STATIC_ASSERT(CONFIGURED_BAUDRATE != -1, "CONFIG_ESP_CONSOLE_UART_BAUDRATE is not defined!");

#define USC_DRIVER_TEST                (1)

#if (USC_DRIVER_TEST == 1)
    #define NEEDS_SERIAL_KEY (1)
    
#else
    #define NEEDS_SERIAL_KEY (0)

#endif

#define TASK_PRIORITY_START             ( ( UBaseType_t ) ( 10 ) ) // Used for the serial communication task
#define TASK_STACK_SIZE               (2048) // Used for saving in the heap for FREERTOS
#define TASK_CORE_READER                 (1) // Core 0 is used for all other operations other than wifi or any wireless protocols
#define TASK_CORE_ACTION                 (0)

#define SERIAL_REQUEST_DELAY_MS        (30)
#define SERIAL_KEY_RETRY_DELAY_MS      (50)
#define LOOP_DELAY_MS                  (10)

#define OVERDRIVER_MAX                  (3)

#define MEMORY_BLOCK_MAX               (20)

#define DELAY_MILISECOND_50            (50) // 1 second delay


#define SEMAPHORE_DELAY pdMS_TO_TICKS(3) // 3ms delay for the semaphore

#ifdef __cplusplus
}
#endif

#endif // __USC_DRIVER_CONFIG_H