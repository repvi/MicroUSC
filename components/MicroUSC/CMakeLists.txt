file(GLOB MICROUSC_SOURCES
    "internal/*.c"
    "MicroUSC_Driver/*.c"
    "MicroUSC_Kernel/*.c"
)

idf_component_register(
    COMPONENTS
        internal
        MicroUSC_Driver
        MicroUSC_Kernel
    SRCS ${MICROUSC_SOURCES}
    
    PRIV_INCLUDE_DIRS
        "internal/include"

    INCLUDE_DIRS
        "MicroUSC_Driver/include"
        "MicroUSC_Kernel/include"

    REQUIRES
        driver
        freertos
        esp_system
        debug
)

target_compile_definitions(
    ${COMPONENT_LIB}
    PUBLIC
        DRIVER_MAX=3
    PRIVATE 
        MICROUSC_SYSTEM_PRIORITY=1
        MICROUSC_CORE=0
)