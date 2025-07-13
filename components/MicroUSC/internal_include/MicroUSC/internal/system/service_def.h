#pragma once

#include "MicroUSC/system/uscsystemdef.h"
#include "stddef.h"
#include "stdint.h"
// can be used custom api
typedef struct {
    union {
        uint32_t caller_pc;
        char *data;
    } type;
    microusc_status status;
} MiscrouscBackTrack_t;