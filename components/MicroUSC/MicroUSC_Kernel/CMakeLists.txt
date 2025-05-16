set(src
    memory_pool.c
    MicroUSC-kernel.c
    status.c
)

idf_component_register(
    SRCS ${src}
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        freertos
        esp_system
        debug
    
     PRIV_REQUIRES
        internal
)