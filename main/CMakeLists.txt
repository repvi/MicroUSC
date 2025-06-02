#file(GLOB_RECURSE srcs "*.c" # do not use, instead use set()

set(srcs
    main.c
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES
        debug
        esp_timer
        nvs_flash
        MicroUSC
        examples
        bootloader 
        bootloader_support
)

#idf_component_get_property(tusb_lib leeebo__tinyusb_src COMPONENT_LIB)

#component_compile_options(-w)