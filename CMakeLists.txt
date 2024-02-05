cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(DRSSTC_Firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
        main.cpp
        lcd.h
        inputs.h
        hw_config.c
        gui.h
        player.h
        util.h
        transmitter.h
)

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
            pico_stdlib
            hardware_pwm
            pico_time
            hardware_adc
            FatFs_SPI
            hardware_clocks
            pico_multicore
)