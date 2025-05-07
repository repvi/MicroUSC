idf_component_register(
    SRC_DIRS  "status"
    SRCS "tiny_kernel.c"
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        memory_storage
        freertos
        esp_system
        debug
        driver_connection
        memory_storage
)