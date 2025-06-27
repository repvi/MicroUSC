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
 * @brief Gets the last length of JSON data
 * @return Last length of JSON data
 */
int get_json_last_len(void);

/**
 * @brief Placeholder free function for pool allocator
 * Memory is freed when pool is reset
 */
void my_pool_free(void *ptr);

/**
 * @brief Resets the pool offset to reuse pool memory
 */
void cjson_pool_reset(void);


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

/**
 * @brief Safely extracts a string from a cJSON object
 * 
 * Retrieves a string value from a cJSON object using the specified key.
 * Performs null checks and type validation before returning the value.
 *
 * @param item The cJSON object to search in
 * @param key  The key to look up
 * @return     String value if found and valid, NULL otherwise
 *
 * Example:
 * For JSON: {"name": "sensor1"}
 * get_cjson_string(root, "name") returns "sensor1"
 */
char *get_cjson_string(cJSON *item, char *const key);

/**
 * @brief Safely extracts an integer from a cJSON object
 * 
 * Retrieves an integer value from a cJSON object using the specified key.
 * Performs null checks and type validation before returning the value.
 *
 * @param item The cJSON object to search in
 * @param key  The key to look up
 * @return     Integer value if found and valid, -1 otherwise
 *
 * Example:
 * For JSON: {"value": 42}
 * get_cjson_int(root, "value") returns 42
 */
int get_cjson_int(cJSON *item ,char *const key);

#ifdef __cplusplus
}
#endif