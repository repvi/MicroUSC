#include "MicroUSC/wireless/parsing.h"
#include "esp_log.h"
#include "esp_system.h"

/* Pool configuration */
#define CJSON_POOL_SIZE 1024
#define TAG "[CJSON POOL]"

uint8_t cjson_pool[CJSON_POOL_SIZE];
size_t cjson_pool_offset = 0;

int get_json_last_len(void)
{
    return cjson_pool_offset; // Return the current offset as the last length
}

/**
 * @brief Allocates memory from the pool
 * @param sz Size to allocate
 * @return Pointer to allocated memory, or NULL if pool is full
 */
static void *my_pool_malloc(size_t sz) 
{
    if (cjson_pool_offset + sz > CJSON_POOL_SIZE) {
        return NULL; // Out of memory!
    }
    void *ptr = &cjson_pool[cjson_pool_offset];
    cjson_pool_offset += sz;
    return ptr;
}

void my_pool_free(void *ptr) 
{
    // No-op: can't free individual blocks in bump allocator
    (void)ptr;
}

void cjson_pool_reset(void) 
{
    cjson_pool_offset = 0;
}

void setup_cjson_pool(void) 
{
    cJSON_Hooks hooks = {
        .malloc_fn = my_pool_malloc,
        .free_fn = my_pool_free
    };
    cJSON_InitHooks(&hooks);
}

cJSON *check_cjson(char *const data, size_t data_len) 
{
    return cJSON_ParseWithLength(data, data_len);
}

char *get_cjson_string(cJSON *item, char *const key)
{
    cJSON *child = cJSON_GetObjectItemCaseSensitive(item, key);
    if (child && cJSON_IsString(child) && child->valuestring) {
        return child->valuestring;
    }
    else {
        return NULL; // Return NULL if not a string or not found
    }
}

int get_cjson_int(cJSON *item ,char *const key) 
{
    cJSON *child = cJSON_GetObjectItemCaseSensitive(item, key);
    if (child && cJSON_IsNumber(child) && child->valueint) {
        return child->valueint;
    }
    else {
        return -1; // Return -1 if not a number or not found
    }
}