#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

unsigned int calculate_checksum(unsigned int value);

void set_rtc_cycle(void);

void increment_rtc_cycle(void);

esp_err_t init_system_memory_space(void);

#ifdef __cplusplus
}
#endif