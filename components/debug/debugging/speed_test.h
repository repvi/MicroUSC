#pragma once

#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>

#define TAG "[CHECKPOINT]"

#define CHECK_FUNCTION_SPEED(func) do {                                       \
        uint64_t start = esp_timer_get_time();                                \
        (func);                                                               \
        uint64_t end = esp_timer_get_time();                                  \
        ESP_LOGI("[TIMING]", #func " took %llu microseconds", end - start);   \
    } while (0)

#define CHECK_FUNCTION_SPEED_WITH_DEBUG(func) do {                            \
        uint64_t start = esp_timer_get_time();                                \
        esp_err_t ret = (func);                                               \
        uint64_t end = esp_timer_get_time();                                  \
        ESP_LOGI("[TIMING]", "Function: %s, Return Value: %d", #func, ret);   \
        if ((ret) != ESP_OK) {                                                \
            ESP_LOGE("[TIMING]", "Function: %s", #func);                      \
        }                                                                     \
        ESP_LOGI("[TIMING]", #func " took %llu microseconds", end - start);   \
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
#undef TIME_TAG