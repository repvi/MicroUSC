#ifndef __TINY_KERNEL_H
#define __TINY_KERNEL_H

#include "USC_driver_config.h"
#include "initStart.h"
#include "status.h" // make available for the developer
#include "esp_system.h"

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

#endif // __TINY_KERNEL_H