# USC Driver Management API Documentation

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![Component](https://img.shields.io/badge/Component-MicroUSC%2FDriver-orange)](../../components/MicroUSC/include/MicroUSC/USCdriver.h)

## Overview

The USC Driver Management component provides the main interface for initializing, managing, and communicating with UART-based drivers in the MicroUSC system. It integrates with FreeRTOS, memory pools, and atomic operations for robust embedded system design with thread-safe data transmission and event-driven architecture.

## Files

- **Header**: `components/MicroUSC/include/MicroUSC/USCdriver.h`
- **Implementation**: `components/MicroUSC/src/application/USCdriver.c`
- **Internal Storage**: `components/MicroUSC/internal_include/MicroUSC/internal/driverList.h` (Internal use only)

## Key Features

- **UART Driver Initialization**: Configurable baud rates, GPIO pins, and callback functions
- **Thread-Safe Communication**: Atomic operations and FreeRTOS integration
- **Event-Driven Design**: Callback-based data processing architecture
- **Memory Pool Integration**: Efficient memory management for embedded constraints
- **Security Protocol Support**: Password-based authentication mechanisms
- **Dynamic Driver Management**: Runtime driver registration and management
- **Linked List Storage**: Internal driver organization (not publicly accessible)

## Architecture

### Driver Storage
Drivers are managed internally using a **linked list structure** defined in `driverList.h`. This internal component is **not accessible** to external users and provides:

- Dynamic driver registration
- Thread-safe driver enumeration
- Memory-efficient storage
- Task lifecycle management

### Task Architecture
Each driver creates two FreeRTOS tasks:
- **Reader Task**: Handles UART data reception
- **Processor Task**: Processes received data through callback functions

### Memory Management
- Static memory pools for driver structures
- Configurable stack sizes per driver
- Automatic memory cleanup on driver removal

## API Reference

### Type Definitions

#### `usc_process_t`
```c
typedef void (*usc_process_t)(void *);
```

**Description**: Function pointer type for driver data processing callbacks.

**Parameters**:
- `void *`: Pointer to the driver instance that received data

**Usage**: Define custom data processing logic for each driver instance.

#### `uscDriverHandler`
```c
typedef struct usc_driver_t *uscDriverHandler;
```

**Description**: Opaque handle to a USC driver instance.

**Notes**:
- Internal structure is not accessible to external code
- Use provided API functions to interact with driver instances
- Handle remains valid until driver is properly deinitialized

### Configuration Macros

#### `STANDARD_UART_CONFIG`
```c
#define STANDARD_UART_CONFIG { \
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE, \
        .data_bits = UART_DATA_8_BITS, \
        .parity    = UART_PARITY_DISABLE, \
        .stop_bits = UART_STOP_BITS_1, \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
    }
```

**Description**: Standard UART configuration using SDK configuration baud rate.

**Usage**: Use as a starting point for UART configuration or when standard settings are sufficient.

### Driver Management Functions

#### `usc_driver_install()`
```c
esp_err_t usc_driver_install(const char *const driver_name,
                            const uart_config_t uart_config, 
                            const uart_port_config_t port_config, 
                            const usc_process_t driver_process,
                            const stack_size_t stack_size);
```

**Description**: Initialize and install a UART-based driver with specified configurations, GPIO pins, and data processing callback.

**Parameters**:
- `driver_name`: Optional identifier for the driver (nullable). If NULL, a default name is used
- `uart_config`: UART configuration structure defining baud rate, data bits, parity, etc.
- `port_config`: UART port configuration specifying GPIO mappings and buffer sizes
- `driver_process`: Callback function for handling received UART data (**must not be NULL**)
- `stack_size`: Stack size for UART driver task (recommended: appropriate for expected data loads)

**Returns**:
- `ESP_OK`: Successfully initialized the driver
- `ESP_ERR_INVALID_ARG`: Invalid GPIO settings in `port_config` or `driver_process` is NULL
- `ESP_FAIL`: Invalid `uart_config` or failure in UART driver installation

**Example**:
```c
// Define data processing callback
void sensor_data_processor(void *driver_instance) {
    uscDriverHandler driver = (uscDriverHandler)driver_instance;
    uint32_t data = usc_driver_get_data(driver);
    
    if (data != 0) {
        ESP_LOGI("SENSOR", "Received: %lu", data);
        // Process sensor data
        process_sensor_reading(data);
    }
}

// Configure UART
uart_config_t uart_config = STANDARD_UART_CONFIG;
uart_config.baud_rate = 115200;

uart_port_config_t port_config = {
    .port = UART_NUM_1,
    .rx_pin = GPIO_NUM_16,
    .tx_pin = GPIO_NUM_17,
    .tx_buffer_size = 256,
    .rx_buffer_size = 256
};

// Install driver
esp_err_t err = usc_driver_install("temperature_sensor", 
                                  uart_config, 
                                  port_config, 
                                  sensor_data_processor, 
                                  2048);
if (err != ESP_OK) {
    ESP_LOGE("DRIVER", "Failed to install driver: %s", esp_err_to_name(err));
}
```

**Important Notes**:
- Verify assigned GPIO pins don't conflict with other peripherals
- Choose appropriate stack size based on expected UART traffic
- Driver must be properly deinitialized when no longer needed
- Callback function runs in driver task context

**Callback Function Guidelines**:
- Keep processing minimal to avoid blocking UART reception
- Use queues or message passing for complex processing
- Avoid blocking operations in callback
- Handle errors gracefully

### Data Communication Functions

#### `usc_driver_get_data()`
```c
uint32_t usc_driver_get_data(uscDriverHandler driver);
```

**Description**: Retrieve UART data from the specified driver instance.

**Parameters**:
- `driver`: Handle to the initialized UART driver

**Returns**:
- The data buffer received via UART as uint32_t
- If driver is invalid or uninitialized, behavior is undefined

**Usage**:
```c
void data_callback(void *driver_instance) {
    uscDriverHandler driver = (uscDriverHandler)driver_instance;
    uint32_t received_data = usc_driver_get_data(driver);
    
    if (received_data != 0) {
        // Process the received data
        handle_received_data(received_data);
    }
}
```

**Thread Safety**: Function is thread-safe when called from the driver's callback context.

**Notes**:
- This is the correct API method for extracting UART data
- Internal driver structure elements are not accessible
- Ensure driver instance is initialized and valid before calling

#### `usc_send_data()`
```c
esp_err_t usc_send_data(uscDriverHandler driver, uint32_t data);
```

**Description**: Send data through the specified UART driver.

**Parameters**:
- `driver`: Handle to the initialized UART driver
- `data`: 32-bit data to transmit via UART

**Returns**:
- `ESP_OK`: Data sent successfully
- `ESP_ERR_INVALID_ARG`: Invalid driver handle
- `ESP_FAIL`: Transmission failed

**Usage**:
```c
void send_sensor_command(uscDriverHandler driver, uint32_t command) {
    esp_err_t result = usc_send_data(driver, command);
    if (result != ESP_OK) {
        ESP_LOGE("DRIVER", "Failed to send command: %s", esp_err_to_name(result));
    }
}

// Send a sensor wake-up command
send_sensor_command(my_driver, 0x12345678);
```

**Thread Safety**: Function is thread-safe and can be called from any task context.

### System Status Functions

#### `usc_print_driver_configurations()`
```c
void usc_print_driver_configurations(void);
```

**Description**: Print the current configuration settings for all drivers managed by the MicroUSC system.

**Usage**: 
- Debugging and diagnostics
- Verification of driver parameters
- Hardware interface settings confirmation
- System startup diagnostics

**Example Output**:
```
[DRIVER_STATUS] Driver Count: 2
[DRIVER_STATUS] Driver 1: temperature_sensor
  - Port: UART_NUM_1
  - Baud Rate: 115200
  - RX Pin: GPIO_16
  - TX Pin: GPIO_17
  - Status: ACTIVE
[DRIVER_STATUS] Driver 2: pressure_sensor
  - Port: UART_NUM_2
  - Baud Rate: 9600
  - RX Pin: GPIO_18
  - TX Pin: GPIO_19
  - Status: ACTIVE
```

**Usage**:
```c
// Display all driver configurations
usc_print_driver_configurations();

// Trigger via system status (alternative method)
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
```

## Internal Driver Management

### Driver Storage (Internal)
The driver system uses an internal linked list implementation that is **not accessible** to external users:

**File**: `components/MicroUSC/internal_include/MicroUSC/internal/driverList.h`

**Features**:
- Dynamic driver registration and removal
- Thread-safe linked list operations
- Memory pool allocation for driver structures
- Task lifecycle management
- Driver state tracking

**Internal Functions** (not accessible to external code):
- `usc_drivers_pause()`: Pause all driver tasks
- `usc_drivers_resume()`: Resume all driver tasks
- Driver list manipulation functions
- Memory management functions

### Driver Lifecycle
1. **Installation**: Driver registered in internal linked list
2. **Task Creation**: Reader and processor tasks created
3. **Active Operation**: Continuous UART monitoring and data processing
4. **Pause/Resume**: System-level control of all drivers
5. **Cleanup**: Automatic resource cleanup on system shutdown

## Configuration Examples

### Basic Temperature Sensor
```c
void temperature_callback(void *driver_instance) {
    uscDriverHandler driver = (uscDriverHandler)driver_instance;
    uint32_t temp_data = usc_driver_get_data(driver);
    
    // Convert and process temperature
    float temperature = (float)temp_data / 100.0f;
    ESP_LOGI("TEMP", "Temperature: %.2f°C", temperature);
}

esp_err_t setup_temperature_sensor(void) {
    uart_config_t uart_cfg = STANDARD_UART_CONFIG;
    uart_cfg.baud_rate = 9600;
    
    uart_port_config_t port_cfg = {
        .port = UART_NUM_1,
        .rx_pin = GPIO_NUM_16,
        .tx_pin = GPIO_NUM_17,
        .tx_buffer_size = 128,
        .rx_buffer_size = 128
    };
    
    return usc_driver_install("temp_sensor", uart_cfg, port_cfg, 
                             temperature_callback, 1024);
}
```

### High-Speed Data Logger
```c
void data_logger_callback(void *driver_instance) {
    uscDriverHandler driver = (uscDriverHandler)driver_instance;
    uint32_t log_data = usc_driver_get_data(driver);
    
    // Queue data for processing in separate task
    if (xQueueSend(log_queue, &log_data, 0) != pdPASS) {
        ESP_LOGW("LOGGER", "Log queue full, dropping data");
    }
}

esp_err_t setup_data_logger(void) {
    // Create processing queue
    log_queue = xQueueCreate(100, sizeof(uint32_t));
    
    uart_config_t uart_cfg = STANDARD_UART_CONFIG;
    uart_cfg.baud_rate = 460800;  // High speed
    
    uart_port_config_t port_cfg = {
        .port = UART_NUM_2,
        .rx_pin = GPIO_NUM_18,
        .tx_pin = GPIO_NUM_19,
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024
    };
    
    return usc_driver_install("data_logger", uart_cfg, port_cfg, 
                             data_logger_callback, 4096);
}
```

### Multi-Driver System
```c
esp_err_t setup_multi_driver_system(void) {
    esp_err_t result;
    
    // Initialize system first
    init_MicroUSC_system();
    
    // Setup multiple drivers
    result = setup_temperature_sensor();
    if (result != ESP_OK) return result;
    
    result = setup_pressure_sensor();
    if (result != ESP_OK) return result;
    
    result = setup_humidity_sensor();
    if (result != ESP_OK) return result;
    
    // Print all configurations for verification
    usc_print_driver_configurations();
    
    return ESP_OK;
}
```

## Security Features

The driver system includes built-in security protocols for device authentication:

### Authentication Protocol
- **Password Request**: `REQUEST_KEY_VAL` (0x64)
- **Ping Operation**: `PING_VAL` (0x63)
- **Password Transmission**: `SEND_KEY_VAL` (1234)
- **Internal Authentication**: `SERIAL_KEY_VAL` (1234)

### Security Functions (Internal)
These functions are implemented internally but not exposed in the public API:
- `usc_driver_request_password()`: Request authentication from remote device
- `usc_driver_ping()`: Connectivity verification
- `usc_driver_send_password()`: Transmit authentication credentials

## Performance Considerations

### Memory Optimization
- Use appropriate stack sizes for driver tasks
- Configure UART buffer sizes based on data rates
- Consider memory pool allocation for frequent operations

### Task Scheduling
- Driver tasks run at system-defined priorities
- Minimize callback processing time
- Use queues for complex data processing

### UART Configuration
- Choose appropriate baud rates for data requirements
- Configure buffer sizes to prevent overflow
- Consider hardware flow control for high-speed applications

## Error Handling

### Common Error Scenarios
1. **Invalid GPIO Configuration**: Verify pin assignments and conflicts
2. **Buffer Overflow**: Increase UART buffer sizes or reduce data rate
3. **Task Stack Overflow**: Increase stack size for driver tasks
4. **Callback Errors**: Implement error handling in callback functions

### Debug Strategies
```c
// Enable debug logging
#define USC_DRIVER_DEBUG

// Check driver status
usc_print_driver_configurations();

// Monitor system status
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);
send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
```

## Best Practices

### Initialization Order
```c
void app_main(void) {
    // 1. Initialize MicroUSC system
    init_MicroUSC_system();
    
    // 2. Install drivers
    setup_temperature_sensor();
    setup_pressure_sensor();
    
    // 3. Verify configuration
    usc_print_driver_configurations();
}
```

### Callback Design
- Keep callbacks lightweight and non-blocking
- Use queues for complex processing
- Handle all error conditions
- Avoid recursive calls or infinite loops

### Resource Management
- Verify GPIO pin assignments before installation
- Monitor memory usage during development
- Use appropriate stack sizes for expected workloads
- Implement proper error recovery

## Troubleshooting

### Common Issues

1. **Driver Installation Fails**
   - Check GPIO pin availability and conflicts
   - Verify UART port is not already in use
   - Ensure sufficient memory for driver tasks

2. **No Data Received**
   - Verify UART configuration matches remote device
   - Check GPIO pin connections
   - Monitor UART buffer status

3. **Callback Not Called**
   - Verify driver installation succeeded
   - Check if driver tasks are running
   - Monitor system task status

4. **Memory Issues**
   - Increase stack size for driver tasks
   - Check memory pool allocation
   - Monitor heap usage

### Debug Commands
```c
// System diagnostics
send_microusc_system_status(USC_SYSTEM_SPECIFICATIONS);
send_microusc_system_status(USC_SYSTEM_MEMORY_USAGE);
send_microusc_system_status(USC_SYSTEM_DRIVER_STATUS);

// Driver-specific diagnostics
usc_print_driver_configurations();
```

## Related Components

- **[System Manager](system_manager.md)**: Core system initialization and status management
- **[Memory Management](memory_management.md)**: Memory pool allocation for drivers
- **[Sleep Management](sleep.md)**: Power management integration
- **[Status Monitoring](status.md)**: System and driver status reporting

---

**Author**: repvi
**Last Updated**: July 13, 2025  
**Version**: ESP-IDF v5.4
