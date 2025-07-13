# Memory Management API Documentation

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![Component](https://img.shields.io/badge/Component-MicroUSC%2FMemory-orange)](../../components/MicroUSC/include/MicroUSC/system/memory_pool.h)

## Overview

The Memory Management component provides IRAM-optimized memory pool management for ESP32/ESP8266 embedded systems. It features fixed-size block allocation with ESP32-specific optimizations, including IRAM placement, cache-friendly alignment, and deterministic memory access suitable for FreeRTOS environments and interrupt handlers.

## Files

- **Header**: `components/MicroUSC/include/MicroUSC/system/memory_pool.h`
- **Implementation**: `components/MicroUSC/src/system/memory_pool.c`
- **System Init**: `components/MicroUSC/internal_include/MicroUSC/internal/system/init.h` (Internal)
- **Driver Memory**: `components/MicroUSC/internal_include/MicroUSC/internal/driverList.h` (Internal)

## Key Features

- **Fixed-Size Block Allocation**: Preallocated memory pools to prevent fragmentation
- **ESP32-Specific Optimizations**: IRAM placement and cache-friendly alignment
- **Thread-Safe Operations**: Safe for FreeRTOS tasks and ISR contexts
- **Deterministic Performance**: O(1) allocation/deallocation for real-time systems
- **Memory Placement Control**: Explicit control over DRAM, PSRAM, and IRAM usage
- **Embedded-Optimized**: Designed for constrained environments with minimal overhead
- **System Integration**: Seamless integration with MicroUSC driver system

## Memory Architecture

### Memory Pool Structure
```c
typedef struct {
    memory_block_t *memory_free;  // Head of free block linked list
    void *memory;                 // Base memory region
    size_t block_size;           // Size of each block in bytes
    size_t num_blocks;           // Total number of blocks
    size_t free_blocks;          // Number of available blocks (tracking)
} memory_pool_t;
```

### Memory Block Structure
```c
typedef struct memory_block_t {
    struct memory_block_t *next; // Next free block pointer
} memory_block_t;
```

### Memory Placement Options
```c
typedef enum {
    DRAM = MALLOC_CAP_8BIT,      // Standard DRAM memory
    PSRAM = MALLOC_CAP_SPIRAM,   // External PSRAM (if available)
    IRAM = MALLOC_CAP_32BIT,     // Internal RAM (faster access)
} memory_placement_t;
```

## Type Definitions

### `memory_pool_t`
Core memory pool structure containing all metadata and pointers for pool management.

### `memory_block_handle_t`
```c
typedef memory_pool_t *memory_block_handle_t;
```
Handle type for referencing memory pools in function calls.

### `serial_data_t`
```c
typedef char serial_data_t[15];
```
Fixed-size character array type for serial data storage.

## API Reference

### Pool Initialization Functions

#### `memory_pool_init()`
```c
bool memory_pool_init(memory_pool_t *pool, const size_t block_size, const size_t num_blocks);
```

**Description**: Initialize a memory pool for efficient block allocation with ESP32-specific optimizations.

**Parameters**:
- `pool`: Pointer to pre-allocated `memory_pool_t` structure
- `block_size`: Size of each memory block in bytes (must be ≥ sizeof(void*))
- `num_blocks`: Total number of blocks in the pool

**Returns**:
- `true`: Pool initialized successfully
- `false`: Invalid parameters or allocation failure

**Usage**:
```c
// Static pool allocation
static memory_pool_t my_pool;

// Initialize pool with 32-byte blocks, 100 blocks total
bool success = memory_pool_init(&my_pool, 32, 100);
if (!success) {
    ESP_LOGE("MEMORY", "Failed to initialize memory pool");
    return ESP_ERR_NO_MEM;
}

ESP_LOGI("MEMORY", "Pool initialized: %d blocks of %d bytes", 100, 32);
```

**Memory Layout**:
```
Pool Structure:
┌─────────────────┐
│ memory_pool_t   │ ← Pool metadata
├─────────────────┤
│ Block 0 (32B)   │ ← First block
├─────────────────┤
│ Block 1 (32B)   │ ← Second block
├─────────────────┤
│     ...         │
├─────────────────┤
│ Block 99 (32B)  │ ← Last block
└─────────────────┘
Total: 3200 bytes + metadata
```

**Important Notes**:
- Must be called once before any pool operations
- Subsequent calls on initialized pools cause undefined behavior
- Use `IRAM_ATTR` if pool will be accessed from interrupts
- Proper alignment is automatically enforced

#### `memory_pool_malloc()`
```c
memory_pool_t *memory_pool_malloc(const size_t block_size, const size_t num_blocks) __attribute__((malloc));
```

**Description**: Dynamically allocate and initialize a memory pool in IRAM with proper alignment and ESP32 optimizations.

**Parameters**:
- `block_size`: Size of each memory block in bytes (must be ≥ sizeof(void*))
- `num_blocks`: Total number of blocks in the pool

**Returns**:
- `memory_pool_t*`: Pointer to initialized pool on success
- `NULL`: Allocation failure or invalid parameters

**Usage**:
```c
// Dynamic pool allocation
memory_pool_t *dynamic_pool = memory_pool_malloc(64, 50);
if (!dynamic_pool) {
    ESP_LOGE("MEMORY", "Failed to create dynamic pool");
    return ESP_ERR_NO_MEM;
}

ESP_LOGI("MEMORY", "Dynamic pool created: %d blocks of %d bytes", 50, 64);

// Use pool...
void *block = memory_pool_alloc(dynamic_pool);

// Cleanup when done
memory_pool_destroy(dynamic_pool);
```

**Memory Allocation**:
- Pool structure allocated with `MALLOC_CAP_8BIT | MALLOC_CAP_DMA`
- Cache-friendly access patterns optimized for ESP32
- Automatic alignment to prevent fragmentation

#### `memory_handler_malloc()`
```c
#define memory_handler_malloc(block_size, num_blocks) memory_pool_malloc(block_size, num_blocks);
```

**Description**: Convenience macro wrapping `memory_pool_malloc()` for consistent naming.

**Usage**: Identical to `memory_pool_malloc()` but provides alternative naming convention.

### Memory Allocation Functions

#### `memory_pool_alloc()`
```c
void *memory_pool_alloc(memory_pool_t *pool) __attribute__((malloc));
```

**Description**: Allocate a memory block from the specified memory pool with O(1) performance.

**Parameters**:
- `pool`: Pointer to initialized `memory_pool_t` structure

**Returns**:
- `void*`: Pointer to allocated memory block
- `NULL`: Pool exhausted or invalid pool

**Usage**:
```c
// Allocate blocks from pool
void *block1 = memory_pool_alloc(&my_pool);
void *block2 = memory_pool_alloc(&my_pool);
void *block3 = memory_pool_alloc(&my_pool);

if (block1 && block2 && block3) {
    // Use allocated blocks
    memset(block1, 0, pool.block_size);
    strcpy((char*)block2, "Hello World");
    
    // Process data in block3
    process_data(block3);
} else {
    ESP_LOGW("MEMORY", "Pool exhausted or allocation failed");
}
```

**Performance Characteristics**:
- **Time Complexity**: O(1) constant time allocation
- **Space Overhead**: Minimal (single pointer operation)
- **Thread Safety**: Not inherently thread-safe; use external synchronization
- **ISR Safety**: Safe for use in interrupt contexts if pool is in IRAM

**Pool State Management**:
```c
void demonstrate_pool_usage(memory_pool_t *pool) {
    ESP_LOGI("DEMO", "Free blocks before: %zu", pool->free_blocks);
    
    void *block = memory_pool_alloc(pool);
    if (block) {
        ESP_LOGI("DEMO", "Free blocks after alloc: %zu", pool->free_blocks);
        
        // Use block...
        
        memory_pool_free(pool, block);
        ESP_LOGI("DEMO", "Free blocks after free: %zu", pool->free_blocks);
    }
}
```

#### `memory_pool_free()`
```c
void memory_pool_free(memory_pool_t *pool, void *block);
```

**Description**: Return a memory block to the memory pool for future allocation with O(1) performance.

**Parameters**:
- `pool`: Pointer to initialized `memory_pool_t` structure
- `block`: Pointer to memory block previously allocated from the same pool

**Usage**:
```c
// Allocate and use a block
void *data_block = memory_pool_alloc(&my_pool);
if (data_block) {
    // Use the block
    sprintf((char*)data_block, "Data: %d", sensor_reading);
    process_sensor_data(data_block);
    
    // Return block to pool when done
    memory_pool_free(&my_pool, data_block);
    data_block = NULL; // Good practice to avoid use-after-free
}
```

**Critical Requirements**:
- Block **must** have been allocated from the same pool
- Do not free the same block twice
- Do not use block after freeing
- Pool and block pointers must be valid

**Safe Usage Pattern**:
```c
esp_err_t safe_block_operation(memory_pool_t *pool) {
    void *block = memory_pool_alloc(pool);
    if (!block) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t result = ESP_OK;
    
    // Perform operations on block
    result = process_block_data(block);
    
    // Always free block, even on error
    memory_pool_free(pool, block);
    
    return result;
}
```

### Pool Management Functions

#### `memory_pool_destroy()`
```c
void memory_pool_destroy(memory_pool_t* pool);
```

**Description**: Destroy a memory pool and release all associated resources to prevent memory leaks.

**Parameters**:
- `pool`: Pointer to `memory_pool_t` structure to be destroyed

**Usage**:
```c
// Create and use a temporary pool
memory_pool_t *temp_pool = memory_pool_malloc(128, 20);
if (temp_pool) {
    // Use pool for temporary operations
    perform_temporary_operations(temp_pool);
    
    // Clean up when done
    memory_pool_destroy(temp_pool);
    temp_pool = NULL; // Prevent accidental reuse
}
```

**Important Notes**:
- Pool pointer and all allocated blocks become invalid after destruction
- Automatically handles both pool structure and memory region cleanup
- Safe to call with NULL pointer (no-op)
- **Must not** use pool or any blocks after destruction

**Cleanup Sequence**:
1. Free internal memory region (`heap_caps_free(pool->memory)`)
2. Reset internal pointers to NULL
3. Free pool structure itself
4. All references become invalid

## System Memory Management (Internal)

### System Memory Initialization

#### `init_system_memory_space()` (Internal)
```c
esp_err_t init_system_memory_space(void);
```

**Description**: Initialize complete system memory space including memory handlers, mutexes, and configuration storage.

**Usage**: Called internally by `init_MicroUSC_system()` - not for direct use.

**Internal Operations**:
- Sets up memory handlers and mutexes
- Initializes configuration storage
- Prepares memory subsystem for driver operations

### Driver Memory Management (Internal)

#### `init_driver_list_memory_pool()` (Internal)
```c
esp_err_t init_driver_list_memory_pool(const size_t buffer_size, const size_t data_size);
```

**Description**: Initialize memory pool for driver list nodes and their associated buffers.

**Parameters**:
- `buffer_size`: Size in bytes of buffer for each driver's buffer.memory
- `data_size`: Size in bytes of data buffer (stored for future use)

**Internal Usage**:
```c
// Called during system initialization
init_driver_list_memory_pool(128, 256); // 128-byte buffers, 256-byte data
```

#### `setUSCtaskSize()` (Internal)
```c
esp_err_t setUSCtaskSize(stack_size_t size);
```

**Description**: Initialize static memory pool for driver task stacks.

**Parameters**:
- `size`: Size in bytes of each task stack for every driver

**Internal Usage**:
```c
// Configure stack size for all driver tasks
setUSCtaskSize(2048); // 2KB stack per driver task
```

## Advanced Usage Patterns

### Thread-Safe Pool Management
```c
typedef struct {
    memory_pool_t pool;
    SemaphoreHandle_t mutex;
} thread_safe_pool_t;

esp_err_t thread_safe_pool_init(thread_safe_pool_t *safe_pool, 
                               size_t block_size, size_t num_blocks) {
    // Initialize mutex
    safe_pool->mutex = xSemaphoreCreateMutex();
    if (!safe_pool->mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize pool
    if (!memory_pool_init(&safe_pool->pool, block_size, num_blocks)) {
        vSemaphoreDelete(safe_pool->mutex);
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

void *thread_safe_alloc(thread_safe_pool_t *safe_pool) {
    void *block = NULL;
    
    if (xSemaphoreTake(safe_pool->mutex, portMAX_DELAY) == pdTRUE) {
        block = memory_pool_alloc(&safe_pool->pool);
        xSemaphoreGive(safe_pool->mutex);
    }
    
    return block;
}

void thread_safe_free(thread_safe_pool_t *safe_pool, void *block) {
    if (xSemaphoreTake(safe_pool->mutex, portMAX_DELAY) == pdTRUE) {
        memory_pool_free(&safe_pool->pool, block);
        xSemaphoreGive(safe_pool->mutex);
    }
}
```

### Multi-Size Pool System
```c
typedef struct {
    memory_pool_t small_pool;   // 32-byte blocks
    memory_pool_t medium_pool;  // 128-byte blocks
    memory_pool_t large_pool;   // 512-byte blocks
} multi_size_allocator_t;

esp_err_t multi_pool_init(multi_size_allocator_t *allocator) {
    bool success = true;
    
    success &= memory_pool_init(&allocator->small_pool, 32, 100);
    success &= memory_pool_init(&allocator->medium_pool, 128, 50);
    success &= memory_pool_init(&allocator->large_pool, 512, 20);
    
    return success ? ESP_OK : ESP_ERR_NO_MEM;
}

void *smart_alloc(multi_size_allocator_t *allocator, size_t size) {
    if (size <= 32) {
        return memory_pool_alloc(&allocator->small_pool);
    } else if (size <= 128) {
        return memory_pool_alloc(&allocator->medium_pool);
    } else if (size <= 512) {
        return memory_pool_alloc(&allocator->large_pool);
    }
    return NULL; // Size too large
}
```

### ISR-Safe Memory Pool
```c
// Pool must be in IRAM for ISR access
IRAM_ATTR static memory_pool_t isr_pool;

IRAM_ATTR void setup_isr_pool(void) {
    // Initialize pool for ISR use
    memory_pool_init(&isr_pool, 64, 10);
}

IRAM_ATTR void *isr_alloc(void) {
    // Safe for use in interrupt context
    return memory_pool_alloc(&isr_pool);
}

IRAM_ATTR void isr_free(void *block) {
    // Safe for use in interrupt context
    memory_pool_free(&isr_pool, block);
}
```

## Performance Analysis

### Time Complexity
- **Allocation**: O(1) - Single pointer operation
- **Deallocation**: O(1) - Single pointer operation  
- **Initialization**: O(n) - Where n is number of blocks
- **Destruction**: O(1) - Single free operation

### Space Complexity
- **Overhead per Pool**: sizeof(memory_pool_t) ≈ 20-24 bytes
- **Overhead per Block**: sizeof(void*) ≈ 4 bytes (only when free)
- **Total Overhead**: ~4% for 128-byte blocks, ~25% for 16-byte blocks

### Memory Alignment
- Automatic alignment to prevent fragmentation
- Cache-friendly access patterns
- DMA-compatible memory allocation

## Memory Debugging

### Pool Statistics
```c
void print_pool_stats(memory_pool_t *pool, const char *name) {
    size_t total_memory = pool->block_size * pool->num_blocks;
    size_t used_memory = (pool->num_blocks - pool->free_blocks) * pool->block_size;
    size_t free_memory = pool->free_blocks * pool->block_size;
    
    ESP_LOGI("POOL", "=== %s Pool Statistics ===", name);
    ESP_LOGI("POOL", "Block size: %zu bytes", pool->block_size);
    ESP_LOGI("POOL", "Total blocks: %zu", pool->num_blocks);
    ESP_LOGI("POOL", "Free blocks: %zu", pool->free_blocks);
    ESP_LOGI("POOL", "Used blocks: %zu", pool->num_blocks - pool->free_blocks);
    ESP_LOGI("POOL", "Total memory: %zu bytes", total_memory);
    ESP_LOGI("POOL", "Used memory: %zu bytes", used_memory);
    ESP_LOGI("POOL", "Free memory: %zu bytes", free_memory);
    ESP_LOGI("POOL", "Utilization: %.1f%%", 
             (float)used_memory / total_memory * 100.0f);
}
```

### Memory Leak Detection
```c
typedef struct {
    void *address;
    uint32_t timestamp;
    uint32_t line;
    const char *file;
} allocation_record_t;

static allocation_record_t alloc_records[100];
static size_t record_count = 0;

void *debug_pool_alloc(memory_pool_t *pool, uint32_t line, const char *file) {
    void *block = memory_pool_alloc(pool);
    if (block && record_count < 100) {
        alloc_records[record_count] = (allocation_record_t){
            .address = block,
            .timestamp = esp_timer_get_time(),
            .line = line,
            .file = file
        };
        record_count++;
    }
    return block;
}

#define DEBUG_ALLOC(pool) debug_pool_alloc(pool, __LINE__, __FILE__)
```

## Error Handling

### Common Error Scenarios
```c
esp_err_t robust_pool_operation(memory_pool_t *pool) {
    // Validate pool
    if (!pool || !pool->memory) {
        ESP_LOGE("MEMORY", "Invalid pool pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check pool state
    if (pool->free_blocks == 0) {
        ESP_LOGW("MEMORY", "Pool exhausted");
        return ESP_ERR_NO_MEM;
    }
    
    // Allocate with error checking
    void *block = memory_pool_alloc(pool);
    if (!block) {
        ESP_LOGE("MEMORY", "Allocation failed despite available blocks");
        return ESP_ERR_NO_MEM;
    }
    
    // Use block safely
    esp_err_t result = process_data_safely(block);
    
    // Always free, even on error
    memory_pool_free(pool, block);
    
    return result;
}
```

### Pool Validation
```c
bool validate_pool_integrity(memory_pool_t *pool) {
    if (!pool || !pool->memory) {
        ESP_LOGE("VALIDATE", "Invalid pool pointers");
        return false;
    }
    
    // Count free blocks by traversing list
    size_t counted_free = 0;
    memory_block_t *current = pool->memory_free;
    while (current && counted_free < pool->num_blocks) {
        counted_free++;
        current = current->next;
    }
    
    if (counted_free != pool->free_blocks) {
        ESP_LOGE("VALIDATE", "Free block count mismatch: counted=%zu, stored=%zu",
                 counted_free, pool->free_blocks);
        return false;
    }
    
    ESP_LOGI("VALIDATE", "Pool integrity verified");
    return true;
}
```

## Best Practices

### Pool Sizing Guidelines
```c
// Recommended pool configurations for different use cases

// High-frequency small allocations (sensor data, small messages)
memory_pool_init(&small_pool, 32, 200);

// Medium allocations (UART buffers, JSON objects)  
memory_pool_init(&medium_pool, 256, 50);

// Large allocations (file buffers, network packets)
memory_pool_init(&large_pool, 1024, 10);

// ISR-safe pool (minimal size for interrupt contexts)
memory_pool_init(&isr_pool, 64, 20);
```

### Resource Management
```c
void proper_pool_lifecycle(void) {
    // 1. Initialize pools early in system startup
    memory_pool_t *network_pool = memory_pool_malloc(512, 20);
    
    // 2. Use pools throughout application lifetime
    handle_network_operations(network_pool);
    
    // 3. Clean up during shutdown
    memory_pool_destroy(network_pool);
}
```

### Integration with MicroUSC System
```c
void system_memory_setup(void) {
    // System automatically handles:
    // - Driver memory pools via init_driver_list_memory_pool()
    // - Task stack pools via setUSCtaskSize()
    // - System memory space via init_system_memory_space()
    
    init_MicroUSC_system(); // Handles all internal memory setup
    
    // Application can create additional pools as needed
    memory_pool_t *app_pool = memory_pool_malloc(128, 50);
    
    // Use application-specific pools for custom allocations
    use_application_pools(app_pool);
}
```

## Troubleshooting

### Common Issues

1. **Pool Initialization Fails**
   - Check available heap memory with `esp_get_free_heap_size()`
   - Reduce pool size or block count
   - Verify parameters are non-zero

2. **Allocation Returns NULL**
   - Pool may be exhausted - check `pool->free_blocks`
   - Validate pool pointer and integrity
   - Consider increasing pool size

3. **Memory Corruption**
   - Ensure blocks are only freed to original pool
   - Check for buffer overruns in allocated blocks
   - Validate pool integrity regularly

4. **Performance Issues**
   - Verify pool is properly aligned
   - Check for excessive fragmentation
   - Consider using multiple pools for different sizes

### Debug Commands
```c
// Memory diagnostics
show_memory_usage();                    // System memory overview
print_pool_stats(&my_pool, "MyPool");   // Pool-specific statistics
validate_pool_integrity(&my_pool);      // Integrity checking
```

## Related Components

- **[System Manager](system_manager.md)**: Core system initialization that sets up memory subsystem
- **[Driver Management](driver_management.md)**: Uses memory pools for driver buffers and stacks
- **[Status Monitoring](status.md)**: Memory usage reporting and diagnostics
- **[Sleep Management](sleep.md)**: Memory state preservation across sleep cycles

---

**Author**: repvi
**Last Updated**: July 13, 2025  
**Version**: ESP-IDF v5.4
