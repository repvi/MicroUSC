# MicroUSC - ESP32 Universal Serial Communication Driver System

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-red)](https://www.espressif.com/en/products/socs/esp32)

## Overview

MicroUSC is a comprehensive ESP32 driver system designed for managing multiple UART-based serial communication drivers with built-in task management, memory pooling, and power management features. The system provides a robust framework for embedded applications requiring concurrent serial device communication with system monitoring capabilities.

## Features

### Core Features
- **Multi-Driver UART Management**: Support for multiple concurrent UART drivers with independent task scheduling
- **Memory Pool Allocation**: Efficient static memory management with alignment support
- **Task Synchronization**: Thread-safe driver operations with FreeRTOS semaphores and mutexes
- **Binary Tree Storage**: Fast string-based data storage and retrieval
- **Atomic Operations**: Lock-free operations for high-performance scenarios

### System Management
- **Power Management**: Deep sleep mode with timer and GPIO wakeup sources
- **System Status Monitoring**: Real-time system state tracking and reporting
- **Error Handling**: Comprehensive error management with logging
- **LED Control**: Built-in LED status indication system

### Development Features
- **Debug Support**: Comprehensive logging with configurable levels
- **Memory Debugging**: Pool usage tracking and leak detection
- **Task Monitoring**: Driver task status and performance metrics

## Architecture

```
MicroUSC/
├── components/MicroUSC/
│   ├── include/               # Public headers
│   ├── internal_include/      # Internal headers
│   └── src/
│       ├── application/       # Driver application logic
│       ├── internal/          # Core system components
│       ├── synced_driver/     # Synchronized driver operations
│       └── system/            # System management
├── main/                      # Application entry point
└── docs/                      # Documentation
```

## Quick Start

### Prerequisites
- ESP-IDF v5.4 or later
- ESP32 development board

### Basic Setup

1. **Initialize the system**:
```c
#include "MicroUSC/system/manager.h"

void app_main(void) {
    // Initialize MicroUSC system
    init_MicroUSC_system();
    
    // System is ready for driver registration
}
```

2. **Configure a UART driver**:
```c
#include "MicroUSC/USCdriver.h"

// UART configuration
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

uart_port_config_t port_config = {
    .port = UART_NUM_1,
    .rx_pin = GPIO_NUM_16,
    .tx_pin = GPIO_NUM_17
};

// Install driver
esp_err_t err = usc_driver_init("sensor1", uart_config, port_config, 
                               my_driver_task, 2048);
```

## API Reference

### Core System
- **[System Manager](api/MicroUSC.md)**: System initialization and status management
- **[Driver Management](api/driver_management.md)**: UART driver registration and control

### Power Management
- **[Sleep Management](api/sleep.md)**: Deep sleep configuration and wakeup sources

## Configuration

### Memory Configuration
```c
// Initialize memory pools
setUSCtaskSize(2048);  // Stack size per driver task
init_driver_list_memory_pool(128, 256);  // Buffer and data sizes
```

### System Configuration
```c
// Configure system behavior
send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
```

## Examples

### Complete UART Driver Example
```c
void my_sensor_task(void *pvParameters) {
    struct usc_driver_t *driver = (struct usc_driver_t *)pvParameters;
    
    while (1) {
        uint32_t data = usc_driver_get_data(driver);
        if (data != 0) {
            ESP_LOGI("SENSOR", "Received: %lu", data);
            
            // Process data locally
            process_sensor_data(data);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Performance Considerations

### Memory Optimization
- Use memory pools for frequent allocations
- Static allocation for driver tasks and buffers
- Minimize dynamic memory usage

### Task Scheduling
- Configure appropriate task priorities
- Use core pinning for time-critical operations
- Minimize critical section duration

### Power Efficiency
- Implement proper sleep modes
- Use GPIO wakeup for external events
- Monitor system reboot cycles

## Debugging

### Enable Debug Logging
```c
// In menuconfig or sdkconfig
CONFIG_LOG_DEFAULT_LEVEL=4  // Enable debug logs

// In code
#define MICROUSC_DEBUG_MEMORY_USAGE
```

### Memory Pool Monitoring
```c
// Check pool usage
ESP_LOGI(TAG, "Pool usage: %d/%d", used_blocks, total_blocks);
```

## Troubleshooting

### Common Issues

1. **Task Starvation**: Minimize semaphore hold time
2. **Memory Leaks**: Always pair malloc/free operations
3. **Driver Conflicts**: Ensure unique UART port assignments
4. **Power Management**: Proper wakeup source configuration

### Error Codes
- `ESP_ERR_NO_MEM`: Insufficient memory for allocation
- `ESP_ERR_INVALID_ARG`: Invalid parameter passed to function
- `ESP_FAIL`: General failure (check logs for details)

## Contributing

### Development Workflow
1. Create feature branch from `development`
2. Implement changes with proper documentation
3. Add unit tests for new functionality
4. Submit pull request for review

### Code Style
- Follow ESP-IDF coding standards
- Use comprehensive function documentation
- Implement proper error handling
- Add debug logging for troubleshooting

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/ASM-GP/ESP32_USC_DRIVERS/issues)
- **Documentation**: [Project Wiki](https://github.com/ASM-GP/ESP32_USC_DRIVERS/wiki)
- **Examples**: See `/examples` directory

---

**Author**: repvi
**Project**: MicroUSC
**Last Updated**: July 13, 2025