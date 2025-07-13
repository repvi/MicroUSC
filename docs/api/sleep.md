# Sleep Management API Documentation

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4-blue)](https://github.com/espressif/esp-idf)
[![Component](https://img.shields.io/badge/Component-MicroUSC%2FSleep-orange)](../../components/MicroUSC/include/MicroUSC/system/sleep.h)

## Overview

The Sleep Management component provides comprehensive power management capabilities for the MicroUSC system on ESP32 platforms. It enables deep sleep mode with configurable wakeup sources including timer-based and GPIO-based wakeup mechanisms, allowing for efficient power consumption in battery-powered and low-power embedded applications.

## Files

- **Header**: `components/MicroUSC/include/MicroUSC/system/sleep.h`
- **Implementation**: `components/MicroUSC/src/system/sleep.c`

## Key Features

- **Deep Sleep Mode**: Ultra-low power consumption state
- **Timer Wakeup**: Configurable timer-based wakeup in microseconds
- **GPIO Wakeup**: External signal-triggered wakeup via GPIO pins
- **Flexible Configuration**: Enable/disable individual wakeup sources
- **Default Settings**: Pre-configured safe defaults for quick setup
- **System Integration**: Seamless integration with system status management
- **Power Optimization**: Automatic validation of wakeup source configuration

## Power Management Architecture

### Sleep Configuration Structure
```c
struct sleep_config_t {
    gpio_num_t wakeup_pin;        // GPIO pin for external wakeup
    uint64_t time;                // Timer duration in microseconds
    bool wakeup_pin_enable;       // GPIO wakeup enable flag
    bool sleep_time_enable;       // Timer wakeup enable flag
} deep_sleep;
```

### Wakeup Sources
1. **Timer Wakeup**: Wakes the ESP32 after a specified time duration
2. **GPIO Wakeup**: Wakes the ESP32 when a GPIO pin changes state
3. **Combined Mode**: Both wakeup sources can be enabled simultaneously

### Power States
- **Active Mode**: Normal operation with full CPU and peripheral functionality
- **Deep Sleep Mode**: Ultra-low power state with only RTC and wakeup sources active
- **Wakeup Transition**: System restart after wakeup (program begins from start)

## Configuration Macros

### Time Conversion Utilities

#### `CONVERT_TO_SLEEPMODE_TIME(x)`
```c
#define CONVERT_TO_SLEEPMODE_TIME(x) ( uint64_t ) ( pdMS_TO_TICKS(x) * portTICK_PERIOD_MS * 1000ULL )
```

**Description**: Convert milliseconds to microseconds for sleep timer configuration.

**Parameters**:
- `x`: Time duration in milliseconds

**Returns**: Time duration in microseconds suitable for sleep timer functions

**Usage**:
```c
// Convert 30 seconds to microseconds
uint64_t sleep_time = CONVERT_TO_SLEEPMODE_TIME(30000);
sleep_mode_timer_wakeup(sleep_time);
```

#### `DEFAULT_LIGHTMODE_TIME`
```c
#define DEFAULT_LIGHTMODE_TIME CONVERT_TO_SLEEPMODE_TIME(5000) // 5 seconds
```

**Description**: Default sleep duration of 5 seconds in microseconds.

**Usage**: Used by `sleep_mode_wakeup_default()` for standard sleep configuration.

## API Reference

### Timer Wakeup Functions

#### `sleep_mode_timer_wakeup()`
```c
void sleep_mode_timer_wakeup(uint64_t time);
```

**Description**: Set the timer duration for sleep mode wakeup in microseconds.

**Parameters**:
- `time`: Duration in microseconds until wakeup (max: approximately 292,471 years)

**Usage**:
```c
// Sleep for 30 seconds
sleep_mode_timer_wakeup(30000000);  // 30 seconds in microseconds

// Sleep for 5 minutes
sleep_mode_timer_wakeup(300000000); // 5 minutes in microseconds

// Using conversion macro
sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(60000)); // 1 minute
```

**Notes**:
- Timer configuration is stored but not applied until `sleep_mode()` is called
- Does not automatically enable timer wakeup; use `sleep_mode_timer(true)` to enable
- Microsecond precision allows for very fine-grained timing control

#### `sleep_mode_timer()`
```c
void sleep_mode_timer(bool option);
```

**Description**: Enable or disable timer as a sleep mode wakeup source.

**Parameters**:
- `option`: `true` to enable timer wakeup, `false` to disable

**Usage**:
```c
// Enable timer wakeup
sleep_mode_timer(true);

// Disable timer wakeup
sleep_mode_timer(false);

// Conditional timer setup
if (battery_level > 50) {
    sleep_mode_timer(true);   // Normal sleep intervals
} else {
    sleep_mode_timer(false);  // Disable timer for maximum power saving
}
```

**Integration Example**:
```c
void setup_timer_wakeup(uint32_t sleep_minutes) {
    uint64_t sleep_microseconds = (uint64_t)sleep_minutes * 60 * 1000000;
    
    sleep_mode_timer_wakeup(sleep_microseconds);
    sleep_mode_timer(true);
    
    ESP_LOGI("SLEEP", "Timer wakeup configured for %d minutes", sleep_minutes);
}
```

### GPIO Wakeup Functions

#### `sleep_mode_wakeup_pin()`
```c
void sleep_mode_wakeup_pin(gpio_num_t pin);
```

**Description**: Set a GPIO pin as the wakeup source for sleep mode.

**Parameters**:
- `pin`: GPIO number for wakeup interrupt (use `GPIO_NUM_NC` to disable)

**Supported GPIO Pins**:
- ESP32: GPIO 0, 2, 4, 12-15, 25-27, 32-39
- Not all GPIO pins support ext0 wakeup; refer to ESP32 documentation

**Usage**:
```c
// Set GPIO 0 as wakeup pin (common for button input)
sleep_mode_wakeup_pin(GPIO_NUM_0);

// Set GPIO 33 as wakeup pin (sensor interrupt)
sleep_mode_wakeup_pin(GPIO_NUM_33);

// Disable GPIO wakeup
sleep_mode_wakeup_pin(GPIO_NUM_NC);
```

**Hardware Considerations**:
- Pin should have appropriate pull-up/pull-down resistors
- Signal should be stable and noise-free
- Consider debouncing for mechanical switches

#### `sleep_mode_wakeup_pin_status()`
```c
void sleep_mode_wakeup_pin_status(bool option);
```

**Description**: Enable or disable GPIO wakeup interrupt pin.

**Parameters**:
- `option`: `true` to enable GPIO wakeup, `false` to disable

**Usage**:
```c
// Enable GPIO wakeup
sleep_mode_wakeup_pin_status(true);

// Disable GPIO wakeup
sleep_mode_wakeup_pin_status(false);

// Conditional GPIO wakeup
if (user_button_enabled) {
    sleep_mode_wakeup_pin_status(true);
} else {
    sleep_mode_wakeup_pin_status(false);
}
```

**Complete GPIO Wakeup Setup**:
```c
void setup_button_wakeup(gpio_num_t button_pin) {
    // Configure GPIO pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << button_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Configure as wakeup source
    sleep_mode_wakeup_pin(button_pin);
    sleep_mode_wakeup_pin_status(true);
    
    ESP_LOGI("SLEEP", "Button wakeup configured on GPIO %d", button_pin);
}
```

### System Configuration Functions

#### `sleep_mode_wakeup_default()`
```c
void sleep_mode_wakeup_default(void);
```

**Description**: Configure default wakeup handler for sleep mode with safe default settings.

**Default Configuration**:
- **Timer**: Enabled with 5-second duration (`DEFAULT_LIGHTMODE_TIME`)
- **GPIO**: Disabled (`GPIO_NUM_NC`)

**Internal Operations**:
```c
// Equivalent to:
sleep_mode_timer_wakeup(DEFAULT_LIGHTMODE_TIME);  // 5 seconds
sleep_mode_timer(true);
sleep_mode_wakeup_pin(GPIO_NUM_NC);
sleep_mode_wakeup_pin_status(false);
```

**Usage**:
```c
void quick_sleep_setup(void) {
    // Apply safe defaults
    sleep_mode_wakeup_default();
    
    // System will wake up every 5 seconds
    ESP_LOGI("SLEEP", "Default sleep configuration applied");
}

void system_initialization(void) {
    init_MicroUSC_system();
    
    // Set up default sleep behavior
    sleep_mode_wakeup_default();
    
    // Other system initialization...
}
```

**Customization After Defaults**:
```c
void custom_sleep_setup(void) {
    // Start with defaults
    sleep_mode_wakeup_default();
    
    // Customize as needed
    sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(30000)); // 30 seconds
    sleep_mode_wakeup_pin(GPIO_NUM_0);     // Add button wakeup
    sleep_mode_wakeup_pin_status(true);    // Enable button
}
```

### Sleep Entry Function

#### `sleep_mode()`
```c
void sleep_mode(void);
```

**Description**: Enter deep sleep mode if wakeup sources are configured. The ESP32 will restart when wakeup occurs.

**Behavior**:
- **No Wakeup Sources**: Function returns immediately without entering sleep
- **Timer Only**: Wakes up after configured timer duration
- **GPIO Only**: Wakes up on GPIO pin state change (HIGH level trigger)
- **Both Sources**: Wakes up on whichever condition occurs first

**Power Consumption**:
- **Deep Sleep**: ~10μA (with RTC running)
- **Active Mode**: ~80-240mA (depending on operations)

**Important Notes**:
- ⚠️ **Program restarts after wakeup** - execution begins from `app_main()`
- All RAM contents are lost (except RTC memory)
- GPIO states are maintained during sleep
- UART connections are lost and must be re-established

**Usage**:
```c
void enter_power_save_mode(void) {
    ESP_LOGI("POWER", "Preparing for sleep mode...");
    
    // Save any critical data to RTC memory or flash if needed
    save_system_state();
    
    // Configure wakeup sources
    sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(30000)); // 30 seconds
    sleep_mode_timer(true);
    
    ESP_LOGI("POWER", "Entering sleep mode...");
    sleep_mode(); // System restarts here upon wakeup
}
```

**Validation Logic**:
```c
// Internal validation (illustrative)
void sleep_mode(void) {
    if (!(deep_sleep.sleep_time_enable || deep_sleep.wakeup_pin_enable)) {
        ESP_LOGW("SLEEP", "No wakeup sources configured, sleep cancelled");
        return; // Prevents infinite sleep
    }
    
    // Configure enabled wakeup sources and enter sleep
    // ...
}
```

## Sleep Management Patterns

### Basic Sleep Cycle
```c
void basic_sleep_example(void) {
    // Configure 1-minute timer wakeup
    sleep_mode_timer_wakeup(60000000); // 1 minute in microseconds
    sleep_mode_timer(true);
    
    // Disable GPIO wakeup
    sleep_mode_wakeup_pin_status(false);
    
    ESP_LOGI("SLEEP", "Entering 1-minute sleep cycle");
    sleep_mode();
    
    // This line never executes - system restarts after wakeup
}
```

### Button-Triggered Wakeup
```c
void button_wakeup_example(void) {
    // Configure button on GPIO 0
    setup_button_wakeup(GPIO_NUM_0);
    
    // Disable timer wakeup for indefinite sleep
    sleep_mode_timer(false);
    
    ESP_LOGI("SLEEP", "Sleep until button press...");
    sleep_mode();
    
    // System restarts here when button is pressed
}
```

### Dual Wakeup Sources
```c
void dual_wakeup_example(void) {
    // Configure both timer and GPIO wakeup
    sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(300000)); // 5 minutes
    sleep_mode_timer(true);
    
    setup_button_wakeup(GPIO_NUM_0);
    
    ESP_LOGI("SLEEP", "Sleep for 5 minutes OR until button press");
    sleep_mode();
    
    // Wakes up on whichever condition occurs first
}
```

### Conditional Sleep Management
```c
void smart_sleep_manager(void) {
    bool has_pending_data = check_data_queue();
    int battery_level = read_battery_level();
    
    if (has_pending_data) {
        ESP_LOGI("SLEEP", "Pending data, short sleep");
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(5000)); // 5 seconds
    } else if (battery_level < 20) {
        ESP_LOGI("SLEEP", "Low battery, extended sleep");
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(3600000)); // 1 hour
    } else {
        ESP_LOGI("SLEEP", "Normal operation, standard sleep");
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(60000)); // 1 minute
    }
    
    sleep_mode_timer(true);
    setup_button_wakeup(GPIO_NUM_0); // Always allow button wakeup
    
    sleep_mode();
}
```

## System Integration

### Integration with System Manager
```c
void trigger_sleep_via_status(void) {
    // Sleep can be triggered via system status
    send_microusc_system_status(USC_SYSTEM_SLEEP);
    
    // This will internally call sleep_mode() in the system task
}
```

### Startup Wakeup Reason Detection
```c
void check_wakeup_reason(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI("WAKEUP", "Timer wakeup");
            handle_timer_wakeup();
            break;
            
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI("WAKEUP", "GPIO wakeup");
            handle_gpio_wakeup();
            break;
            
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI("WAKEUP", "Normal startup (not wakeup)");
            handle_normal_startup();
            break;
    }
}

void app_main(void) {
    // Check why we started up
    check_wakeup_reason();
    
    // Initialize system
    init_MicroUSC_system();
    
    // Continue with application logic...
}
```

### Data Persistence Across Sleep
```c
// Use RTC memory for data that survives sleep
RTC_DATA_ATTR int boot_count = 0;
RTC_DATA_ATTR struct {
    uint32_t sensor_reading;
    uint64_t timestamp;
} persistent_data;

void save_data_before_sleep(void) {
    boot_count++;
    persistent_data.sensor_reading = read_sensor();
    persistent_data.timestamp = esp_timer_get_time();
    
    ESP_LOGI("PERSIST", "Boot count: %d, Last reading: %d", 
             boot_count, persistent_data.sensor_reading);
}
```

## Power Optimization Strategies

### Dynamic Sleep Duration
```c
void adaptive_sleep_duration(void) {
    uint32_t sleep_duration;
    
    // Adapt sleep duration based on activity
    if (high_activity_detected()) {
        sleep_duration = 5000;    // 5 seconds
    } else if (medium_activity_detected()) {
        sleep_duration = 30000;   // 30 seconds
    } else {
        sleep_duration = 300000;  // 5 minutes
    }
    
    sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(sleep_duration));
    sleep_mode_timer(true);
    sleep_mode();
}
```

### Battery-Aware Sleep Management
```c
void battery_optimized_sleep(void) {
    int battery_percentage = get_battery_level();
    
    if (battery_percentage > 50) {
        // Normal operation
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(60000)); // 1 minute
        setup_button_wakeup(GPIO_NUM_0); // Allow user interaction
    } else if (battery_percentage > 20) {
        // Power saving mode
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(300000)); // 5 minutes
        setup_button_wakeup(GPIO_NUM_0); // Still allow user interaction
    } else {
        // Critical power saving
        sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(3600000)); // 1 hour
        sleep_mode_wakeup_pin_status(false); // Disable GPIO to save power
    }
    
    sleep_mode_timer(true);
    sleep_mode();
}
```

## Error Handling and Validation

### Safe Sleep Configuration
```c
esp_err_t safe_sleep_setup(uint64_t timer_us, gpio_num_t wakeup_gpio) {
    // Validate timer duration (minimum 1 second, maximum ~24 hours)
    if (timer_us < 1000000 || timer_us > 86400000000ULL) {
        ESP_LOGE("SLEEP", "Invalid timer duration: %llu us", timer_us);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate GPIO pin
    if (wakeup_gpio != GPIO_NUM_NC && !GPIO_IS_VALID_GPIO(wakeup_gpio)) {
        ESP_LOGE("SLEEP", "Invalid GPIO pin: %d", wakeup_gpio);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Configure sleep parameters
    sleep_mode_timer_wakeup(timer_us);
    sleep_mode_timer(true);
    
    if (wakeup_gpio != GPIO_NUM_NC) {
        sleep_mode_wakeup_pin(wakeup_gpio);
        sleep_mode_wakeup_pin_status(true);
    } else {
        sleep_mode_wakeup_pin_status(false);
    }
    
    ESP_LOGI("SLEEP", "Sleep configured: timer=%llu us, gpio=%d", timer_us, wakeup_gpio);
    return ESP_OK;
}
```

### Pre-Sleep System Check
```c
bool verify_sleep_readiness(void) {
    // Check if any critical operations are pending
    if (uart_operations_pending()) {
        ESP_LOGW("SLEEP", "UART operations pending, delaying sleep");
        return false;
    }
    
    // Verify wakeup sources
    if (!deep_sleep.sleep_time_enable && !deep_sleep.wakeup_pin_enable) {
        ESP_LOGE("SLEEP", "No wakeup sources configured!");
        return false;
    }
    
    // Check system resources
    if (esp_get_free_heap_size() < 1024) {
        ESP_LOGW("SLEEP", "Low memory, potential issues after wakeup");
    }
    
    return true;
}

void safe_sleep_entry(void) {
    if (verify_sleep_readiness()) {
        ESP_LOGI("SLEEP", "System ready for sleep");
        sleep_mode();
    } else {
        ESP_LOGW("SLEEP", "Sleep cancelled due to system state");
    }
}
```

## Best Practices

### Initialization Sequence
```c
void sleep_system_init(void) {
    // Always set up defaults first
    sleep_mode_wakeup_default();
    
    // Then customize as needed
    sleep_mode_timer_wakeup(CONVERT_TO_SLEEPMODE_TIME(30000)); // 30 seconds
    
    // Configure hardware-specific wakeup
    setup_button_wakeup(GPIO_NUM_0);
    
    ESP_LOGI("SLEEP", "Sleep system initialized");
}
```

### Resource Management
- Save critical data before sleep (to RTC memory or flash)
- Close file handles and network connections
- Disable unnecessary peripherals
- Configure GPIO states for minimal power consumption

### Debugging Sleep Issues
```c
void debug_sleep_configuration(void) {
    ESP_LOGI("DEBUG", "=== Sleep Configuration Debug ===");
    ESP_LOGI("DEBUG", "Timer enabled: %s", deep_sleep.sleep_time_enable ? "YES" : "NO");
    ESP_LOGI("DEBUG", "Timer duration: %llu us", deep_sleep.time);
    ESP_LOGI("DEBUG", "GPIO enabled: %s", deep_sleep.wakeup_pin_enable ? "YES" : "NO");
    ESP_LOGI("DEBUG", "GPIO pin: %d", deep_sleep.wakeup_pin);
    
    if (!deep_sleep.sleep_time_enable && !deep_sleep.wakeup_pin_enable) {
        ESP_LOGW("DEBUG", "WARNING: No wakeup sources enabled!");
    }
}
```

## Troubleshooting

### Common Issues

1. **System Doesn't Enter Sleep**
   - Verify at least one wakeup source is enabled
   - Check for pending interrupts or tasks
   - Ensure no blocking operations in progress

2. **Immediate Wakeup After Sleep**
   - Check GPIO pin state and configuration
   - Verify pull-up/pull-down resistors
   - Look for electrical noise on wakeup pin

3. **System Doesn't Wake Up (Timer)**
   - Verify timer duration is reasonable
   - Check if timer wakeup is enabled
   - Ensure microsecond conversion is correct

4. **System Doesn't Wake Up (GPIO)**
   - Verify GPIO pin supports ext0 wakeup
   - Check hardware connections
   - Confirm GPIO configuration

### Debug Strategies
```c
void sleep_debug_session(void) {
    ESP_LOGI("DEBUG", "Starting sleep debug session");
    
    // Show current configuration
    debug_sleep_configuration();
    
    // Test timer wakeup
    ESP_LOGI("DEBUG", "Testing 5-second timer wakeup...");
    sleep_mode_timer_wakeup(5000000);
    sleep_mode_timer(true);
    sleep_mode_wakeup_pin_status(false);
    sleep_mode();
    
    // This code never executes - system restarts after wakeup
}
```

## Related Components

- **[System Manager](system_manager.md)**: Sleep mode triggering via status codes
- **[Driver Management](driver_management.md)**: Driver state during sleep/wake cycles
- **[Status Monitoring](status.md)**: System state reporting and diagnostics
- **[Memory Management](memory_management.md)**: Data persistence across sleep cycles

---

**Author**: repvi
**Last Updated**: July 13, 2025  
**Version**: ESP-IDF v5.4
