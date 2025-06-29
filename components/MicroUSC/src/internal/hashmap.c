#include "MicroUSC/internal/hashmap.h"
#include "stdio.h"
#include <string.h>
#include <stdbool.h>

typedef struct {
    char key[MAX_KEY_LENGTH];
    void* value;
    bool occupied;
} hashmap_entry_t;

struct hashmap_t {
    hashmap_entry_t entries[HASHMAP_SIZE];
    uint16_t size;
    uint16_t collisions;
};

// FNV-1a hash function - good for embedded systems (simple and effective)
static uint32_t hash_function(const char* key) {
    uint32_t hash = HASH_SEED;
    while (*key) {
        hash ^= (uint32_t)*key++;
        hash *= 0x01000193;
    }
    return hash;
}

// Initialize hashmap
void hashmap_init(HashMap map) {
    memset(map->entries, 0, sizeof(map->entries));
    map->size = 0;
    map->collisions = 0;
}

HashMap hashmap_create(void) {
    HashMap map = (HashMap)malloc(sizeof(struct hashmap_t));
    if (map) {
        hashmap_init(map);
    }
    return map;
}

// Linear probing for collision resolution
static uint16_t find_slot(HashMap map, const char* key, bool* found) {
    uint32_t hash = hash_function(key);
    uint16_t index = hash & (HASHMAP_SIZE - 1);  // Fast modulo for power of 2
    uint16_t original_index = index;
    *found = false;

    do {
        if (!map->entries[index].occupied) {
            #ifdef DEBUG_HASHMAP
            printf("Found empty slot for key %s at index %d\n", key, index);
            #endif
            return index;  // Found empty slot
        }
        if (strcmp(map->entries[index].key, key) == 0) {
            #ifdef DEBUG_HASHMAP
            printf("Found existing key %s at index %d\n", key, index);
            #endif
            *found = true;
            return index;  // Found existing key
        }
        index = (index + 1) & (HASHMAP_SIZE - 1);  // Linear probe
    } while (index != original_index);  // Stop if we've checked all slots

    return HASHMAP_SIZE;  // Map is full
}

// Insert or update value
bool hashmap_put(HashMap map, const char* key, void* value) {
    if (map->size >= HASHMAP_SIZE || strlen(key) >= MAX_KEY_LENGTH) {
        return false;
    }

    bool found;
    uint16_t index = find_slot(map, key, &found);
    
    if (index >= HASHMAP_SIZE) {
        return false;  // Map is full
    }

    if (!found) {
        strncpy(map->entries[index].key, key, MAX_KEY_LENGTH - 1);
        map->entries[index].key[MAX_KEY_LENGTH - 1] = '\0';
        map->entries[index].occupied = true;
        map->size++;
        #ifdef DEBUG_HASHMAP
        printf("Inserted key %s at index %d\n", key, index);
        #endif
    }
    
    map->entries[index].value = value;
    return true;
}

// Retrieve value
void* hashmap_get(HashMap map, const char* key) {
    bool found;
    uint16_t index = find_slot(map, key, &found);
    
    if (found && index < HASHMAP_SIZE) {
        #ifdef DEBUG_HASHMAP
        printf("Found key %s at index %d\n", key, index);
        #endif
        return map->entries[index].value;
    }

    #ifdef DEBUG_HASHMAP
    printf("Key %s not found in hashmap\n", key);
    #endif
    return NULL;
}

// Remove entry
bool hashmap_remove(HashMap map, const char* key) {
    bool found;
    uint16_t index = find_slot(map, key, &found);
    
    if (found && index < HASHMAP_SIZE) {
        map->entries[index].occupied = false;
        map->size--;
        #ifdef DEBUG_HASHMAP
        printf("Removed key %s from index %d\n", key, index);
        #endif
        return true;
    }
    #ifdef DEBUG_HASHMAP
    printf("Key %s not found for removal\n", key);
    #endif
    return false;
}