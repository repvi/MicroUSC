set(MICROUSC_ESP_CHIP "")

if(IDF_TARGET STREQUAL "esp32")
    list(APPEND MICROUSC_ESP_CHIP "chip_specific/esp32/system_attr.c")

elseif(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND MICROUSC_ESP_CHIP "chip_specific/esp32s3/system_attr.c")

else()
    message(FATAL_ERROR "Unsupported ESP chip variant")
endif()

set(MICROUSC_INTERNAL
    "internal/genList.c"
    "internal/initStart.c"
)

set(MICROUSC_SYNCED_DRIVER
    "synced_driver/atomic_sys_op.c"
    "synced_driver/esp_uart.c"
    "synced_driver/USCdriver.c"
)

set(MICROUSC_SYSTEM
    "system/kernel.c"
    "system/memory_pool.c"
    "system/MicroUSC-internal.c"
    "system/status.c"
)

list(TRANSFORM MICROUSC_ESP_CHIP PREPEND "src/")
list(TRANSFORM MICROUSC_INTERNAL PREPEND "src/")
list(TRANSFORM MICROUSC_SYNCED_DRIVER PREPEND "src/")
list(TRANSFORM MICROUSC_SYSTEM PREPEND "src/")

idf_component_register(
    SRCS 
        ${MICROUSC_ESP_CHIP}
        ${MICROUSC_INTERNAL}
        ${MICROUSC_SYNCED_DRIVER}
        ${MICROUSC_SYSTEM}

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
)

target_compile_definitions(
    ${COMPONENT_LIB}
    PUBLIC
        DRIVER_MAX=3
    PRIVATE 
        MICROUSC_DEBUG=y
        MICROUSC_SYSTEM_PRIORITY=1
        MICROUSC_CORE=0
)