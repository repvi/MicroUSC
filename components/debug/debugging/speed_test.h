#pragma once

#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>

#define TAG "[CHECKPOINT]"

#define CHECK_FUNCTION_SPEED(func) do {                                     \
        uint64_t start = esp_timer_get_time();                              \
        (func);                                                             \
        uint64_t end = esp_timer_get_time();                                \
        printf("[Timing] " #func " took %llu microseconds\n", end - start); \
    } while (0)

#define CHECK_FUNCTION_SPEED_WITH_DEBUG(func) do {                          \
        uint64_t start = esp_timer_get_time();                              \
        esp_err_t ret = (func);                                             \
        uint64_t end = esp_timer_get_time();                                \
        printf("Function: %s, Return Value: %d\n", #func, ret);             \
        if ((ret) != ESP_OK) {                                              \
            ESP_LOGE("FAILED", "Function: %s", #func);                      \
        }                                                                   \
        printf("[Timing] " #func " took %llu microseconds\n", end - start); \
    } while (0)

#define CHECKPOINT_START \
    int check_point_var = 1; \
    ESP_LOGI(TAG, "Starting tests");

#define CHECKPOINT_MESSAGE \
    ESP_LOGI(TAG, "Check point #%d", check_point_var); \
    check_point_var++;

#define CHECKPOINT_END \
    ESP_LOGI(TAG, "Add tests have been completed");

#undef TAG