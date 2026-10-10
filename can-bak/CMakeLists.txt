cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME test)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# 当前工程内的源码目录
set(CANOPEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/CANopenNode")
set(CANOPEN_STM32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/CANopenNode_STM32")
set(CANOPEN_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/App/CANopen")

# 协议模块源码
file(GLOB CANOPEN_PROTOCOL_SOURCES CONFIGURE_DEPENDS
    "${CANOPEN_DIR}/301/*.c"
    "${CANOPEN_DIR}/303/*.c"
    "${CANOPEN_DIR}/305/*.c"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CANOPEN_PROTOCOL_SOURCES}

    "${CANOPEN_DIR}/CANopen.c"
    "${CANOPEN_DIR}/storage/CO_storage.c"

    "${CANOPEN_STM32_DIR}/CO_driver_STM32.c"
    "${CANOPEN_STM32_DIR}/CO_app_STM32.c"
    "${CANOPEN_STM32_DIR}/CO_storageBlank.c"

    "${CANOPEN_APP_DIR}/OD.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${CANOPEN_DIR}"
    "${CANOPEN_STM32_DIR}"
    "${CANOPEN_APP_DIR}"
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Drivers/BSP/can_common.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Drivers/BSP
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
