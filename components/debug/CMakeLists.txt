idf_component_register(  
    SRC "logger.c"
    INCLUDE_DIRS "debugging" "."
    REQUIRES 
    esp_timer
    general
)