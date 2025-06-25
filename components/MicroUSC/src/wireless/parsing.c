#include "MicroUSC/internal/wireless/parsing.h"
#include "esp_log.h"
#include "esp_system.h"

#define CJSON_POOL_SIZE 512

uint8_t cjson_pool[CJSON_POOL_SIZE];
size_t cjson_pool_offset = 0;

void *my_pool_malloc(size_t sz) {
    if (cjson_pool_offset + sz > CJSON_POOL_SIZE) {
        return NULL; // Out of memory!
    }
    void *ptr = &cjson_pool[cjson_pool_offset];
    cjson_pool_offset += sz;
    return ptr;
}

void my_pool_free(void *ptr) {
    // No-op: can't free individual blocks in bump allocator
    (void)ptr;
}

void cjson_pool_reset(void) {
    cjson_pool_offset = 0;
}

void setup_cjson_pool(void) {
    cJSON_Hooks hooks = {
        .malloc_fn = my_pool_malloc,
        .free_fn = my_pool_free
    };
    cJSON_InitHooks(&hooks);
}

cJSON *check_cjson(char *const data, size_t data_len) {
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    if (root == NULL) {
        ESP_LOGE(TAG, "cJSON root is NULL");
    }
    return root;
}