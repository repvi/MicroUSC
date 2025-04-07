idf_component_register(
    SRC_DIRS "src" "critical_cycle.S"
    INCLUDE_DIRS "include" "."
    PRIV_REQUIRES
        freertos
        esp_system
        driver
        memory_storage
)