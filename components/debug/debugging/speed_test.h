#pragma once

#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>

#define CHECK_FUNCTION_SPEED(func) do {                                     \
        uint64_t start = esp_timer_get_time();                              \
        (func);                                                             \
        uint64_t end = esp_timer_get_time();                                \
        printf("[Timing] " #func " took %llu microseconds\n", end - start); \
    } while (0)

#define CHECK_FUNCTION_SPEED_WITH_DEBUG(func, ret) do {                     \
        uint64_t start = esp_timer_get_time();                              \
        ret = (func);                                                       \
        uint64_t end = esp_timer_get_time();                                \
        printf("Function: %s, Return Value: %d\n", #func, ret);    \
        if ((ret) != ESP_OK) {                                              \
            ESP_LOGE("FAILED", "Function: %s", #func);                      \
        }                                                                   \
        printf("[Timing] " #func " took %llu microseconds\n", end - start); \
    } while (0)

    