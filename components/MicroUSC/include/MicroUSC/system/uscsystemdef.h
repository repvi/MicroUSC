#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif