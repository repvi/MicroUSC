#file(GLOB_RECURSE srcs "*.c" # do not use, instead use set()
set(srcs
    main.c
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES
        debug
        driver_connection
        serial_protocol
        esp_timer
)

#idf_component_get_property(tusb_lib leeebo__tinyusb_src COMPONENT_LIB)

#component_compile_options(-w)