# System Status API Documentation

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![Component](https://img.shields.io/badge/Component-MicroUSC%2FStatus-orange)](../../components/MicroUSC/include/MicroUSC/system/status.h)

## Overview

The System Status component provides comprehensive system monitoring, diagnostics, and reporting capabilities for the MicroUSC library. It offers real-time information about driver configurations, system specifications, memory usage, and ESP32 chip information for debugging, verification, and system health monitoring.

## Files

- **Header**: `components/MicroUSC/include/MicroUSC/system/status.h`
- **Implementation**: `components/MicroUSC/src/system/status.c`
- **Status Definitions**: `components/MicroUSC/include/MicroUSC/system/uscsystemdef.h`
- **Driver Status Types**: `components/MicroUSC/internal_include/MicroUSC/internal/uscdef.h` (Internal)

## Key Features

- **Driver Configuration Reporting**: Display all active driver settings and status
- **System Information**: ESP32 chip specifications and capabilities
- **Memory Monitoring**: Real-time heap usage statistics for different memory regions
- **Status Code Management**: Comprehensive status enumeration for system and driver states
- **Thread-Safe Operations**: Safe access to driver information across multiple tasks
- **Diagnostic Tools**: Essential debugging and verification utilities

## Status Types

### System Status Codes (`microusc_status`)
```c
typedef enum {
    USC_SYSTEM_SUCCESS = 0,       ///< Default/idle system state
    USC_SYSTEM_OFF,               ///< System powered off
    USC_SYSTEM_SLEEP,             ///< System in sleep mode
    USC_SYSTEM_PAUSE,             ///< System paused
    USC_SYSTEM_RESUME,            ///< System resumes
    USC_SYSTEM_WIFI_CONNECT,      ///< WiFi connection in progress (deprecated)
    USC_SYSTEM_BLUETOOTH_CONNECT, ///< Bluetooth connection in progress (deprecated)
    USC_SYSTEM_LED_ON,            ///< LED is turned on
    USC_SYSTEM_LED_OFF,           ///< LED is turned off
    USC_SYSTEM_MEMORY_USAGE,      ///< Query or report memory usage
    USC_SYSTEM_SPECIFICATIONS,    ///< Query system specifications
    USC_SYSTEM_DRIVER_STATUS,     ///< Query driver status
    USC_SYSTEM_ERROR,             ///< System error state
    USC_SYSTEM_PRINT_SUCCUSS,     ///< Print success status
} microusc_status;
```

### Driver Status Codes (`usc_status_t`)
```c
typedef enum {
    DRIVER_UNINITALIALIZED,       ///< Driver not yet initialized
    NOT_CONNECTED,                ///< Hardware not detected
    CONNECTED,                    ///< Physical layer established
    DISCONNECTED,                 ///< Graceful termination
    ERROR,                        ///< Unrecoverable fault
    DATA_RECEIVED,                ///< Successful RX completion
    DATA_SENT,                    ///< Successful TX completion
    DATA_SEND_ERROR,              ///< TX failure (checksum/parity)
    DATA_RECEIVE_ERROR,           ///< RX failure (framing/overflow)
    DATA_SEND_TIMEOUT,            ///< TX blocked > SEMAPHORE_WAIT_TIME
    DATA_RECEIVE_TIMEOUT,         ///< RX incomplete by deadline
    DATA_SEND_COMPLETE,           ///< TX confirmed by peer
    DATA_RECEIVE_COMPLETE,        ///< RX validated
    TIME_OUT,                     ///< General operation timeout
} usc_status_t;
```

## API Reference

### Driver Status Reporting

#### `usc_print_driver_configurations()`
```c
void usc_print_driver_configurations(void);
```

**Description**: Print the current configuration settings for all drivers managed by the MicroUSC system to the console.

**Output Information**:
- Driver name and identifier
- UART baud rate configuration
- Current driver status
- UART port assignment
- TX and RX GPIO pin assignments
- Driver count and enumeration

**Thread Safety**: Thread-safe with semaphore protection for driver access.

**Usage**:
```c
// Direct function call
usc_print_driver_configurations();

// Via system status trigger
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
```

**Example Output**:
```
DRIVER       temperature_sensor
Baud Rate    115200
Status       CONNECTED
UART Port    1
UART TX Pin  17
UART RX Pin  16
--------
DRIVER       pressure_sensor
Baud Rate    9600
Status       DATA_RECEIVED
UART Port    2
UART TX Pin  19
UART RX Pin  18
--------
[STATUS] Finished literating drivers
```

**Use Cases**:
- System startup verification
- Debugging driver configuration issues
- Runtime diagnostic monitoring
- Hardware interface verification
- Development and testing validation

**Internal Operation**:
- Iterates through internal driver linked list
- Acquires semaphore lock for each driver (thread-safe access)
- Displays comprehensive configuration details
- Releases semaphore after data retrieval

### System Information Functions

#### `print_system_info()`
```c
void print_system_info(void);
```

**Description**: Display ESP32 chip information including model, core count, and available features.

**Displayed Information**:
- **Chip Model**: ESP32 or other chip types
- **CPU Cores**: Number of available processor cores
- **Features**: Available connectivity options (WiFi/Bluetooth/BLE)

**Usage**:
```c
// Direct function call
print_system_info();

// Via system status trigger
send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);
```

**Example Output**:
```
ESP32 Chip Info:
  Model: ESP32
  Cores: 2
  Features: WiFi/BT/BLE
```

**Use Cases**:
- System capability verification
- Hardware compatibility checking
- Development environment validation
- Feature availability confirmation
- System documentation and logging

### Memory Monitoring Functions

#### `show_memory_usage()`
```c
void show_memory_usage(void);
```

**Description**: Display ESP32 memory usage statistics for DMA-capable and internal memory regions.

**Memory Regions Monitored**:
- **DMA Capable Memory**: Memory accessible by hardware DMA operations (typically limited)
- **Internal SRAM**: Fast internal memory preferred for performance-critical operations

**Platform Support**:
- **Xtensa Architecture**: Full memory statistics available
- **Other Platforms**: Platform notification displayed

**Usage**:
```c
// Direct function call
show_memory_usage();

// Via system status trigger
send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
```

**Example Output**:
```
[MEMORY] DMA capable memory:
[MEMORY]  Total: 160768 bytes
[MEMORY]  Free: 142336 bytes
[MEMORY] Internal memory:
[MEMORY]  Total: 295128 bytes
[MEMORY]  Free: 245672 bytes
```

**Critical Warnings**:
⚠️ **Heap Corruption Detection**: If this function crashes with a `LoadProhibited` exception, it indicates heap corruption has occurred earlier in program execution.

**Common Heap Corruption Causes**:
- Buffer overflows writing past allocated memory boundaries
- Use-after-free operations accessing freed memory
- Double-free operations corrupting heap metadata
- Memory alignment issues causing improper memory access
- Stack overflow damaging heap structures

**Debugging Memory Issues**:
```c
void memory_debug_routine(void) {
    ESP_LOGI("DEBUG", "Checking memory before operation");
    show_memory_usage();
    
    // Perform memory-intensive operation
    critical_memory_operation();
    
    ESP_LOGI("DEBUG", "Checking memory after operation");
    show_memory_usage();
}
```

## Status Code Integration

### System Status Integration
The status reporting functions integrate with the system status management:

```c
void system_diagnostic_check(void) {
    // Check system specifications
    send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);
    
    // Check memory usage
    send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
    
    // Check driver status
    send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
}
```

### Driver Status Monitoring
Monitor individual driver status changes:

```c
void monitor_driver_status(void) {
    // Print current configurations
    usc_print_driver_configurations();
    
    // The output will show current status for each driver:
    // - CONNECTED: Driver is active and responding
    // - DATA_RECEIVED: Driver has received new data
    // - ERROR: Driver encountered an error
    // - etc.
}
```

## Comprehensive System Diagnostics

### Complete System Health Check
```c
void perform_system_health_check(void) {
    ESP_LOGI("HEALTH", "=== System Health Check ===");
    
    // 1. Display system information
    ESP_LOGI("HEALTH", "1. System Information:");
    print_system_info();
    
    // 2. Check memory usage
    ESP_LOGI("HEALTH", "2. Memory Usage:");
    show_memory_usage();
    
    // 3. Check driver configurations
    ESP_LOGI("HEALTH", "3. Driver Status:");
    usc_print_driver_configurations();
    
    ESP_LOGI("HEALTH", "=== Health Check Complete ===");
}
```

### Automated Monitoring Task
```c
void system_monitor_task(void *pvParameters) {
    const TickType_t monitor_period = pdMS_TO_TICKS(30000); // 30 seconds
    
    while (1) {
        ESP_LOGI("MONITOR", "Performing periodic system check");
        
        // Check memory usage for leaks
        show_memory_usage();
        
        // Monitor driver status
        usc_print_driver_configurations();
        
        vTaskDelay(monitor_period);
    }
}
```

### Development Debug Session
```c
void debug_session_info(void) {
    ESP_LOGI("DEBUG", "=== Development Debug Information ===");
    
    // System capabilities
    print_system_info();
    printf("\n");
    
    // Current memory state
    show_memory_usage();
    printf("\n");
    
    // All driver configurations
    usc_print_driver_configurations();
    
    ESP_LOGI("DEBUG", "=== Debug Session Complete ===");
}
```

## Integration with System Manager

### Status-Triggered Reporting
The status functions integrate seamlessly with the system manager:

```c
// Trigger via system status codes
void trigger_status_reports(void) {
    // These will call the corresponding status functions
    send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);    // -> print_system_info()
    send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);      // -> show_memory_usage()
    send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);     // -> usc_print_driver_configurations()
}
```

### Error Handling Integration
```c
void error_context_reporter(void) {
    ESP_LOGE("ERROR", "System error detected, gathering context...");
    
    // Gather system context for error analysis
    print_system_info();
    show_memory_usage();
    usc_print_driver_configurations();
    
    // Trigger error handling
    send_microusc_system_status(USC_SYSTEM_ERROR);
}
```

## Performance Considerations

### Memory Usage
- Status functions use minimal stack space
- Driver iteration is O(n) where n is the number of drivers
- Semaphore acquisition ensures thread safety but may cause brief delays

### Output Performance
- Console output is synchronous and may affect real-time performance
- Consider using these functions primarily during development and startup
- For production systems, implement filtered or periodic reporting

### Thread Safety
- All functions are designed to be thread-safe
- Driver information access is protected by semaphores
- Functions can be called from any task context

## Best Practices

### Development Phase
```c
void development_startup_sequence(void) {
    // Initialize system
    init_MicroUSC_system();
    
    // Install drivers
    setup_all_drivers();
    
    // Verify system state
    ESP_LOGI("STARTUP", "Verifying system configuration...");
    print_system_info();
    show_memory_usage();
    usc_print_driver_configurations();
    
    ESP_LOGI("STARTUP", "System ready for operation");
}
```

### Production Monitoring
```c
void production_health_monitor(void) {
    static uint32_t last_check = 0;
    uint32_t current_time = esp_timer_get_time() / 1000000; // seconds
    
    // Check every 5 minutes
    if (current_time - last_check > 300) {
        show_memory_usage(); // Monitor for memory leaks
        last_check = current_time;
    }
}
```

### Error Diagnostics
```c
void error_diagnostic_dump(void) {
    ESP_LOGE("DIAG", "=== ERROR DIAGNOSTIC DUMP ===");
    
    // Capture complete system state
    print_system_info();
    show_memory_usage();
    usc_print_driver_configurations();
    
    // Additional error context
    ESP_LOGE("DIAG", "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGE("DIAG", "Minimum free heap: %d bytes", esp_get_minimum_free_heap_size());
    
    ESP_LOGE("DIAG", "=== DIAGNOSTIC DUMP COMPLETE ===");
}
```

## Troubleshooting

### Common Issues

1. **Driver Status Shows ERROR**
   - Check UART pin configurations
   - Verify baud rate matches connected device
   - Monitor for electrical connectivity issues

2. **Memory Usage Continuously Increasing**
   - Look for memory leaks in application code
   - Check for unreleased resources
   - Monitor driver buffer usage

3. **Function Crashes with LoadProhibited**
   - Indicates heap corruption
   - Review recent memory operations
   - Check for buffer overflows and use-after-free

4. **Semaphore Timeout in Driver Status**
   - Driver may be blocked in long operation
   - Check for deadlocks in driver callbacks
   - Verify proper semaphore release patterns

### Debug Strategies

#### Memory Leak Detection
```c
void memory_leak_detector(void) {
    static size_t baseline_free = 0;
    
    size_t current_free = esp_get_free_heap_size();
    
    if (baseline_free == 0) {
        baseline_free = current_free;
        ESP_LOGI("LEAK", "Memory baseline set: %d bytes", baseline_free);
    } else {
        int32_t difference = current_free - baseline_free;
        if (difference < -1024) { // More than 1KB lost
            ESP_LOGW("LEAK", "Potential memory leak: %d bytes lost", -difference);
            show_memory_usage();
        }
    }
}
```

#### Driver Status Verification
```c
bool verify_all_drivers_connected(void) {
    // This would require extending the API to return status codes
    // For now, use visual inspection of printed output
    ESP_LOGI("VERIFY", "Checking driver connection status:");
    usc_print_driver_configurations();
    
    // Manual verification required from console output
    return true;
}
```

## Related Components

- **[System Manager](system_manager.md)**: Status code management and system control
- **[Driver Management](driver_management.md)**: Driver installation and data communication
- **[Memory Management](memory_management.md)**: Memory pool allocation and management
- **[Sleep Management](sleep.md)**: Power management and system state control

---

**Author**: revpi
**Last Updated**: July 13, 2025  
**Version**: ESP-IDF v5.4
