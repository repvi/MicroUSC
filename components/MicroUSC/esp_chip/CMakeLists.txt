set(SRCS )


if(IDF_TARGET STREQUAL "esp32")
    list(APPEND SRCS "esp32/system_atr.c")

elseif(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND SRCS "esp32s3/system_atr.c")

else()
    message(FATAL_ERROR "Custom Error: This component only supports ESP32 and ESP32-S3 targets. Please select a supported chip type.")

endif()

idf_component_register (
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
)