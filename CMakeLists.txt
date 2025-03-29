# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Specify the extra components directory
#set(EXTRA_COMPONENT_DIRS components)
# Include the ESP-IDF project configuration
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(PARTITION_TABLE_CSV "partitions.csv")

project(ESP32_USC_SERIAL_DRIVERS)

# idf.py -p <COM_PORT> erase_flash

# Add the main component
#add_subdirectory(main)
#add_subdirectory(components)

# Custom compiler flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG -Og")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -Og")

# Custom linker flags
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Add custom build steps or custom targets if needed
#add_custom_target(clean-all
#    COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
#)

# Add custom pre-build or post-build commands
#add_custom_command(TARGET ${COMPONENT_LIB} PRE_BUILD
#    COMMAND echo "Running pre-build steps..."
#)

#add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
#    COMMAND echo "Running post-build steps..."
#)