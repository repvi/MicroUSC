#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

void *my_pool_malloc(size_t sz);

void my_pool_free(void *ptr);

void cjson_pool_reset(void);

void setup_cjson_pool(void);

cJSON *check_cjson(char *const data, size_t data_len);

#ifdef __cplusplus
}
#endif