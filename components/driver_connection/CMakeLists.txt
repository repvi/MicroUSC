idf_component_register(
    SRCS "critical_cycle.S"
    SRC_DIRS "src"
    INCLUDE_DIRS "include" "."
    PRIV_REQUIRES
        memory_storage
        freertos
        esp_system
        driver
)