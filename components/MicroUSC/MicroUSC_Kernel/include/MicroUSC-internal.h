#pragma once

#include "USC_driver_config.h" // should rename

#ifdef __cplusplus
extern "C" {
#endif

extern int __microusc_system_code;

esp_err_t microusc_system_task(void);

#ifdef __cplusplus
}
#endif