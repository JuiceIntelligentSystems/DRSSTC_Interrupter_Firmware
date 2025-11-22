# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/boards)

set(PICO_PLATFORM rp2040)
set(PICO_BOARD rp2040_interrupter CACHE STRING "Board type")

# Pull Rpi Pico SDK
include(pico_sdk_import.cmake)

project(DRSSTC_Interrupter_Firmware C CXX ASM)

# Init Pico SDK
pico_sdk_init()

add_executable(DRSSTC_Interrupter_Firmware)
target_sources(DRSSTC_Interrupter_Firmware PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/hw_config.c
        )

pico_set_program_name(DRSSTC_Interrupter_Firmware "DRSSTC_Interrupter_Firmware")
pico_set_program_version(DRSSTC_Interrupter_Firmware "0.2")

pico_enable_stdio_usb(DRSSTC_Interrupter_Firmware 1)

# Add FATFS Library Directory to the build
add_subdirectory(lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build)

target_link_libraries(${PROJECT_NAME} PUBLIC
            pico_stdlib
            hardware_pwm
            pico_time
            hardware_adc
            no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
            hardware_clocks
            pico_multicore
)

# Add Standard include files to the build
target_include_directories(DRSSTC_Interrupter_Firmware PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(${PROJECT_NAME})