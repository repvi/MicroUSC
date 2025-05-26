#pragma once

#include "MicroUSC/system/uscsystemdef.h"
#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_builtin_led(void);

void builtin_led_set(bool state);

void builtin_led_system(microusc_status status);

#ifdef __cplusplus
}
#endif