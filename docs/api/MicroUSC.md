# System Manager API Documentation

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![Component](https://img.shields.io/badge/Component-MicroUSC%2FSystem-orange)](../../components/MicroUSC/include/MicroUSC/system/manager.h)

## Overview

The System Manager is the core component of the MicroUSC library responsible for system initialization, power management, error handling, and status monitoring on ESP32 platforms. It provides a centralized task-based system for managing all MicroUSC operations with thread-safe communication and robust error recovery.

## Files

- **Header**: `components/MicroUSC/include/MicroUSC/system/manager.h`
- **Implementation**: `components/MicroUSC/src/system/manager.c`

## Key Features

- **Single Initialization**: Must be called exactly once at startup
- **Task-Based Architecture**: Dedicated system task for handling status changes
- **Priority Inheritance**: Proper FreeRTOS task priority management  
- **Resource Tracking**: Memory and IRQ usage monitoring
- **Power Management**: Deep sleep and wakeup source configuration
- **Error Handling**: Custom and default error handler management
- **Status Monitoring**: Real-time system state tracking
- **GPIO ISR Management**: Interrupt-driven status updates

## Core Architecture

### System Structure
```c
struct {
    struct {
        StackType_t stack[TASK_STACK_SIZE];
        StaticTask_t taskBuffer;
        TaskHandle_t main_task;
    } task;
    struct {
        QueueHandle_t queue_handler;
        size_t count;
    } queue_system;
    struct {
        microusc_error_handler operation;
        void *stored_var;
        int size;
    } error_handler;
    portMUX_TYPE critical_lock;
} microusc_system;
```

### Task Priority Configuration
- **System Task**: `MICROUSC_SYSTEM_PRIORITY`
- **Core Affinity**: `MICROUSC_CORE` (typically Core 0)
- **Stack Size**: `INTERNAL_TASK_STACK_SIZE` (4096 bytes)

## API Reference

### Type Definitions

#### `microusc_error_handler`
```c
typedef void(*microusc_error_handler)(void *);
```

**Description**: Function pointer type for custom error handlers.

**Parameters**:
- `void *`: Optional data pointer passed to the error handler

**Usage**: Used with `set_microusc_system_error_handler()` to register custom error handling functions.

### Initialization Functions

#### `init_MicroUSC_system()`
```c
void init_MicroUSC_system(void);
```

**Description**: Initialize the MicroUSC system kernel. This function must be called first during system startup before using any other MicroUSC library features.

**Usage Protocol**:
1. Call as the very first line in your `app_main()`
2. Do not re-initialize or call again
3. After initialization, all driver and memory functions are available

**Internal Operations**:
- Initializes system memory pools
- Sets up system task and queue
- Configures GPIO ISR service
- Initializes built-in LED (if enabled)
- Sets up RTC and sleep mode defaults
- Installs default error handler

**Example**:
```c
void app_main(void) {
    // Initialize MicroUSC system - MUST BE FIRST
    init_MicroUSC_system();
    
    // Now safe to use other MicroUSC functions
    usc_driver_init("sensor", uart_config, port_config, task_func, 2048);
}
```

**Warnings**:
- ⚠️ Calling after other MicroUSC initialization causes resource conflicts
- ⚠️ Multiple calls will cause memory corruption and unpredictable behavior

### Status Management

#### `send_microusc_system_status()`
```c
void send_microusc_system_status(microusc_status code);
```

**Description**: Send a status code to the system task for processing. This triggers system behavior changes based on the status code.

**Parameters**:
- `code`: Status code from `microusc_status` enum

**Thread Safety**: Thread-safe with critical sections and queue overflow protection

**Status Codes**:
- `USC_SYSTEM_OFF`: Triggers system restart
- `USC_SYSTEM_SLEEP`: Enters deep sleep mode
- `USC_SYSTEM_PAUSE`: Pauses all drivers
- `USC_SYSTEM_RESUME`: Resumes all drivers
- `USC_SYSTEM_LED_ON/OFF`: Controls built-in LED
- `USC_SYSTEM_MEMORY_USAGE`: Displays memory information
- `USC_SYSTEM_SPECIFICATIONS`: Shows system specifications
- `USC_SYSTEM_DRIVER_STATUS`: Prints driver configurations
- `USC_SYSTEM_ERROR`: Triggers error handler

**Example**:
```c
// Pause all drivers
send_microusc_system_status(USC_SYSTEM_PAUSE);

// Show memory usage
send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);

// Trigger error handling
send_microusc_system_status(USC_SYSTEM_ERROR);
```

**Queue Management**:
- Uses internal queue with overflow protection
- Automatically flushes queue after 3 consecutive overflows
- Non-blocking queue operations prevent task starvation

### Error Handling

#### `set_microusc_system_error_handler()`
```c
void set_microusc_system_error_handler(microusc_error_handler handler, void *var, int size);
```

**Description**: Register a custom error handler for system-level errors.

**Parameters**:
- `handler`: Function pointer matching `microusc_error_handler` signature
- `var`: Optional data to pass to handler (copied internally)
- `size`: Size of data to copy (0 if no data)

**Handler Signature**:
```c
typedef void(*microusc_error_handler)(void *);
```

**Example**:
```c
void my_error_handler(void *data) {
    ESP_LOGE("APP", "Custom error handler called");
    // Perform cleanup
    save_critical_data();
    // System will restart after handler returns
}

// Register custom handler
set_microusc_system_error_handler(my_error_handler, NULL, 0);
```

#### `set_microusc_system_error_handler_default()`
```c
void set_microusc_system_error_handler_default(void);
```

**Description**: Restore the default system error handler.

**Default Handler Behavior**:
- Logs error message
- Prints driver configurations
- Pauses all drivers
- Flushes output buffers
- Triggers system restart

### GPIO ISR Management

#### `microusc_system_isr_pin()`
```c
void microusc_system_isr_pin(gpio_config_t io_config, microusc_status trigger_status);
```

**Description**: Configure a GPIO pin as an interrupt source that triggers system status changes.

**Parameters**:
- `io_config`: GPIO configuration structure
- `trigger_status`: Status code to send when interrupt occurs

**Operation**:
1. Extracts GPIO pin from `pin_bit_mask`
2. Removes any existing ISR handler
3. Configures GPIO with provided settings
4. Installs new ISR handler
5. ISR sends status code to system queue

**Example**:
```c
// Configure GPIO 0 as wakeup button
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE
};

// Trigger resume when button pressed
microusc_system_isr_pin(io_conf, USC_SYSTEM_RESUME);
```

### System Control

#### `microusc_system_restart()`
```c
__attribute__((noreturn)) void microusc_system_restart(void);
```

**Description**: Immediately restart the ESP32 system.

**Behavior**:
- Triggers hardware reset
- Function never returns
- All unsaved data is lost

#### `microusc_infloop()`
```c
__attribute__((noreturn)) void microusc_infloop(void);
```

**Description**: Enter an infinite loop, effectively halting system execution.

**Use Cases**:
- Error handling when recovery is impossible
- System halt in unrecoverable states
- Debug/development purposes

### Deprecated Functions

#### `microusc_system_isr_trigger()` [DEPRECATED]
```c
__deprecated void microusc_system_isr_trigger(void);
```

**Description**: This function is deprecated and will be removed in future versions.

**Status**: Currently empty implementation (TODO)
**Replacement**: Use `microusc_system_isr_pin()` for GPIO interrupt configuration

## System Task Operation

### Task Behavior
The system task runs continuously, processing status codes from the internal queue:

```c
static void microusc_system_task(void *p) {
    MiscrouscBackTrack_t sys_data;
    while (1) {
        if (xQueueReceive(microusc_system.queue_system.queue_handler, 
                         &sys_data, portMAX_DELAY) == pdPASS) {
            // Process status code
            switch(sys_data.status) {
                case USC_SYSTEM_OFF:
                    microusc_system_restart();
                    break;
                case USC_SYSTEM_SLEEP:
                    builtin_led_system(USC_SYSTEM_SLEEP);
                    sleep_mode();
                    break;
                // ... other cases
            }
        }
    }
}
```

### Queue Management
- **Size**: `MICROUSC_QUEUEHANDLE_SIZE` items
- **Item Type**: `MiscrouscBackTrack_t`
- **Overflow Protection**: Automatic flush after 3 overflows
- **Thread Safety**: Critical sections protect queue operations

## Memory Management

### Internal Memory Pools
The system initializes memory pools during startup:
- Driver task stacks
- Queue handlers
- Error handler data storage
- System buffers

### Memory Monitoring
Use `USC_SYSTEM_MEMORY_USAGE` status to display:
- Heap usage statistics
- Pool allocation status
- Stack high water marks

## Configuration

### Build Configuration
```c
// Stack size for system task
#define INTERNAL_TASK_STACK_SIZE (4096)

// Queue size for status messages
#define MICROUSC_QUEUEHANDLE_SIZE 10

// System task priority
#define MICROUSC_SYSTEM_PRIORITY 5

// Core affinity
#define MICROUSC_CORE 0
```

### Debug Features
Enable debugging with:
```c
#define MICROUSC_DEBUG
#define MICROUSC_DEBUG_MEMORY_USAGE
```

## Error Handling Patterns

### Basic Error Handling
```c
void setup_error_handling(void) {
    // Use default handler
    set_microusc_system_error_handler_default();
    
    // Trigger error for testing
    send_microusc_system_status(USC_SYSTEM_ERROR);
}
```

### Custom Error Recovery
```c
typedef struct {
    int error_count;
    uint32_t last_error_time;
} error_context_t;

void recovery_error_handler(void *data) {
    error_context_t *ctx = (error_context_t *)data;
    
    ctx->error_count++;
    ctx->last_error_time = esp_timer_get_time();
    
    if (ctx->error_count < 3) {
        ESP_LOGW("RECOVERY", "Attempting recovery, count: %d", ctx->error_count);
        // Attempt recovery
        send_microusc_system_status(USC_SYSTEM_RESUME);
    } else {
        ESP_LOGE("RECOVERY", "Max retries exceeded, restarting");
        microusc_system_restart();
    }
}

void setup_recovery_handler(void) {
    static error_context_t error_ctx = {0};
    set_microusc_system_error_handler(recovery_error_handler, 
                                     &error_ctx, sizeof(error_ctx));
}
```

## Best Practices

### Initialization Order
```c
void app_main(void) {
    // 1. Initialize MicroUSC system FIRST
    init_MicroUSC_system();
    
    // 2. Set up error handling
    set_microusc_system_error_handler_default();
    
    // 3. Configure GPIO interrupts
    microusc_system_isr_pin(button_config, USC_SYSTEM_RESUME);
    
    // 4. Initialize drivers
    usc_driver_init("sensor", uart_config, port_config, task_func, 2048);
}
```

### Status Code Usage
- Use specific status codes for different operations
- Avoid rapid-fire status updates that could overflow the queue
- Implement proper error recovery patterns

### Thread Safety
- All public functions are thread-safe
- Critical sections protect shared resources
- Queue operations use non-blocking calls when appropriate

## Troubleshooting

### Common Issues

1. **System Won't Initialize**
   - Ensure `init_MicroUSC_system()` is called first
   - Check memory constraints
   - Verify ESP-IDF version compatibility

2. **Queue Overflow**
   - Reduce frequency of status updates
   - Check system task priority
   - Monitor queue usage patterns

3. **ISR Handler Issues**
   - Verify GPIO configuration
   - Check pin bit mask calculation
   - Ensure proper interrupt type

4. **Error Handler Not Called**
   - Verify handler registration
   - Check if system task is running
   - Monitor queue status

### Debug Commands
```c
// Show system information
send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);

// Display memory usage
send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);

// Show driver status
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
```

## Related Components

- **[Sleep Management](sleep.md)**: Deep sleep and wakeup configuration
- **[RTC Management](rtc.md)**: Real-time clock and persistent storage
- **[Driver Management](driver_management.md)**: UART driver registration

---

**Author**: Alejandro Ramirez  
**Last Updated**: July 13, 2025  
**Version**: ESP-IDF v5.4
