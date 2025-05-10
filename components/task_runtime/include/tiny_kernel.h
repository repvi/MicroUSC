#ifndef __TINY_KERNEL_H
#define __TINY_KERNEL_H

#include "USC_driver_config.h"
#include "generic.h"
#include "status_var.h" // make available for the developer
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "[TINY KERNEL]"

void init_tiny_kernel(void); // RUN_FIRST

#ifdef __cplusplus
}
#endif

#endif // __TINY_KERNEL_H