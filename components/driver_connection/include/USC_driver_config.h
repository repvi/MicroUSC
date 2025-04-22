#ifNdef __USC_DRIVER_CONFIG_H
#define __USC_DRIVER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define USC_DRIVER_TEST                (1)


#if (USC_DRIVER_TEST == 1)
    #define NEEDS_SERIAL_KEY (1) // Used for testing purposes

#else
    #define NEEDS_SERIAL_KEY (0) // Used for testing purposes

#endif


#define CURRENT_VERSION_MAJOR             (0)
#define CURRENT_VERSION_MINOR             (6)
#define CURRENT_VERSION_PATCH             (2)

#define to_string(x)        #x
#define USC_Version()       to_string(CURRENT_VERSION_MAJOR) "." \
                            to_string(CURRENT_VERSION_MINOR) "." \
                            to_string(CURRENT_VERSION_PATCH)

#undef to_string

#define DRIVER_MAX              2

#define SERIAL_KEY      "123456789" // Password for the program
#define REQUEST_KEY     "GSKx" // Used for requesting the passcode for the driver
#define PING            "ping" // Used for making sure there is a connection
#define SERIAL_REQUEST_SIZE  (sizeof(uint32_t)) 

#define SERIAL_DATA_SIZE      126

#define ESP32_ARCHITECTURE_ALIGNMENT_VAR (sizeof(uint32_t)) // used for the ESP32 architecture

#if defined(__GNUC__) || defined(__clang__)
    #define RUN_FIRST      __attribute__((constructor)) // probably might not use
    #define MALLOC         __attribute__((malloc)) // used for dynamic memory functions
    #define HOT            __attribute__((hot)) // for critical operations (Need most optimization)
    #define COLD           __attribute__((cold)) // not much much. Use less memory but slower execution of function. Used like in initializing
    
    #define UNUSED __attribute__((unused))
    #define DEPRECATED __attribute__((deprecated))
    #define USED __attribute__((used))

    #define OPT0         __attribute__((optimize("O0")))
    #define OPT1         __attribute__((optimize("O1")))
    #define OPT2         __attribute__((optimize("O2")))
    #define OPT3         __attribute__((optimize("O3")))
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
#endif

#define OPTIMIZE_CONSTANT(x) \
    (__builtin_constant_p(x) ? optimize_for_constant(x) : general_case(x))

    
#define INSIDE_SCOPE(x, max) (0 <= (x) && (x) < (max))
#define OUTSIDE_SCOPE(x, max) ((x) < 0 || (max) <= (x))
#define developer_input(x) (x)

// The literate_bytes macro defines a for loop that iterates from 0 to x - 1, where x is the number of iterations specified.
#define literate_bytes(x) for (size_t i = 0; i < (x); i++)
#define cycle_drivers() for (int i = 0; i < DRIVER_MAX; i++) // used for the driver loop
#define cycle_overdrivers() for (int i = 0; i < OVERDRIVER_MAX; i++) // used for the overdriver loop
// DRAM_ATTR // put in IRAM, not in flash, not in PSRAM

// needs baud rate implementation
#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       CONFIG_ESP_CONSOLE_UART_BAUDRATE
#endif
#ifndef CONFIG_ESP_CONSOLE_UART_BAUDRATE
    #define CONFIGURED_BAUDRATE       (-1)
#endif

ESP_STATIC_ASSERT(CONFIGURED_BAUDRATE != -1, "CONFIG_ESP_CONSOLE_UART_BAUDRATE is not defined!");

#ifdef __cplusplus
}
#endif

#endif