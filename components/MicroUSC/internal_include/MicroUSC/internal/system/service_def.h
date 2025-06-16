#pragma once

// can be used custom api
typedef struct {
    uint32_t caller_pc;
    microusc_status status;
} MiscrouscBackTrack_t;

union MicrouscSystemData_t {
    MiscrouscBackTrack_t backtrack;
    char *data;
};