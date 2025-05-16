set(src
    initStart.c
)
    
idf_component_register(
    SRCS ${src}
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        memory_storage
        freertos
        esp_system
        debug
)