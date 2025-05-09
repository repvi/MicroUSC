#ifndef __TINY_KERNEL_H
#define __TINY_KERNEL_H

#include "USC_driver_config.h"
#include "generic.h"
#include "status.h" // make available for the developer

#ifdef __cplusplus
extern "C" {
#endif

void RUN_FIRST set_system_drivers(void);

#ifdef __cplusplus
}
#endif

#endif // __TINY_KERNEL_H