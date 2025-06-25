#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EMBEDDED_HASHMAP_H
#define EMBEDDED_HASHMAP_H

#include <stdint.h>
#include <stdbool.h>

// Configurable settings
#define HASHMAP_SIZE 8          // Must be power of 2
#define MAX_KEY_LENGTH 16
#define HASH_SEED 0x12345678    // Initial hash seed

typedef struct hashmap_t *HashMap;

void hashmap_init(HashMap map);

bool hashmap_put(HashMap map, const char* key, void* value);

void* hashmap_get(HashMap map, const char* key);

bool hashmap_remove(HashMap map, const char* key);

#endif

#ifdef __cplusplus
}
#endif