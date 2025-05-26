#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate the task handler for the usc drivers for both drivers
 * and overdrivers
 * @return ESP_OK if the the allocation is successful
8*/
esp_err_t init_memory_handlers(void);

void init_tiny_kernel(void); // RUN_FIRST

#ifdef __cplusplus
}
#endif