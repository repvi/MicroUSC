#pragma once

#include "esp_timer.h"
#include <stdio.h>

#define CHECK_FUNCTION_SPEED(func) do {\
        uint64_t start = esp_timer_get_time(); \
        (func); \
        uint64_t end = esp_timer_get_time(); \
        printf("[Timing] " #func " took %llu microseconds\n", end - start); \
    } while (0)