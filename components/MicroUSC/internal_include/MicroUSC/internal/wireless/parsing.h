/**
 * @file parsing.c
 * @brief Memory pool implementation for cJSON parsing
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

/**
 * @brief Sets up cJSON to use the pool allocator
 */
void setup_cjson_pool(void);

/**
 * @brief Parse JSON data with length check
 * @param data JSON string to parse
 * @param data_len Length of JSON string
 * @return Parsed cJSON object or NULL on failure
 */
cJSON *check_cjson(char *const data, size_t data_len);

#ifdef __cplusplus
}
#endif