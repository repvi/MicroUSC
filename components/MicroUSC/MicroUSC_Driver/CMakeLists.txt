set(src
    atomic_sys_op.c
    esp_uart.c
    USCdriver.c
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