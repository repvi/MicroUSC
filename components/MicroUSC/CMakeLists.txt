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
    INCLUDE_DIRS
        "internal/include"
        "MicroUSC_Driver/include"
        "MicroUSC_Kernel/include"
    REQUIRES
        driver
        freertos
        esp_system
        debug
)