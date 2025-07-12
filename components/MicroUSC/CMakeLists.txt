set(COMPILE_DEFINITIONS_LIST)

set(MICROUSC_APPLICATION
    "application/USCdriver.c"
)

set(MICROUSC_ESP_CHIP "")

if(IDF_TARGET STREQUAL "esp32")
    list(APPEND COMPILE_DEFINITIONS_LIST BUILTIN_LED_ASSEMBLY)
    list(APPEND MICROUSC_ESP_CHIP "chip_specific/esp32/system_attr.c")
    list(APPEND MICROUSC_ESP_CHIP "chip_specific/esp32/builtin_led.S")

elseif(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND MICROUSC_ESP_CHIP "chip_specific/esp32s3/system_attr.c")

else()
    message(FATAL_ERROR "Unsupported ESP chip variant")
endif()

set(MICROUSC_INTERNAL
    #"internal/genList.c"
    "internal/driverList.c"
    "internal/system/bit_manip.c"
    "internal/system/init.c"
    "internal/hashmap.c"
    "internal/wireless/wifi_handler.cpp"
    "internal/wireless/mqtt_handler.cpp"
)

set(MICROUSC_SYNCED_DRIVER
    "synced_driver/atomic_sys_op.c"
    "synced_driver/esp_uart.c"
)

set(MICROUSC_SYSTEM
    "system/memory_pool.c"
    "system/manager.c"
    "system/rtc.c"
    "system/status.c"
    "system/sleep.c"
)

set(MICROUSC_WIRELESS
    "wireless/wifi.cpp"
    "wireless/mqtt.cpp"
    "wireless/ota.c"
    "wireless/parsing.c"
)

list(TRANSFORM MICROUSC_APPLICATION PREPEND "src/")
list(TRANSFORM MICROUSC_ESP_CHIP PREPEND "src/")
list(TRANSFORM MICROUSC_INTERNAL PREPEND "src/")
list(TRANSFORM MICROUSC_SYNCED_DRIVER PREPEND "src/")
list(TRANSFORM MICROUSC_SYSTEM PREPEND "src/")
list(TRANSFORM MICROUSC_WIRELESS PREPEND "src/")

idf_component_register(
    COMPILE_OPTIONS "-mtext-section-literals"

    SRCS 
        ${MICROUSC_APPLICATION}
        ${MICROUSC_ESP_CHIP}
        ${MICROUSC_INTERNAL}
        ${MICROUSC_SYNCED_DRIVER}
        ${MICROUSC_SYSTEM}
        ${MICROUSC_WIRELESS}

    INCLUDE_DIRS
        "include"

    PRIV_INCLUDE_DIRS
        "internal_include"
    
    REQUIRES
        driver
        freertos
        esp_system
        debug
        led_strip
        esp_wifi
        esp_http_client
        mqtt
        general

    PRIV_REQUIRES
        esp_event
        nvs_flash
        esp_https_ota
        driver
        json
)

target_compile_definitions(
    ${COMPONENT_LIB}
    PUBLIC
        DRIVER_MAX=3

    PRIVATE 
        ${COMPILE_DEFINITIONS_LIST}
        MICROUSC_DEBUG
        MICROUSC_UART_DEBUG
        MICROUSC_DEBUG_MEMORY_USAGE
        MICROUSC_SYSTEM_PRIORITY=1
        MICROUSC_CORE=0
        MICROUSC_QUEUEHANDLE_SIZE=5
        UART_QUEUE_SIZE=10
        BUFFER_SIZE=1024
        SEND_BUFFER_SIZE=6
        MICROUSC_MQTT_DEBUG
        SYSTEM_WIFI_DEBUG
        DEBUG_HASHMAP
)

# add_compile_options(-mtext-section-literals)