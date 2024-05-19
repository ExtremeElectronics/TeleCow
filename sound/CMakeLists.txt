cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

project(sound)

add_library(sound INTERFACE)

target_include_directories(sound INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(sound INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/sound.c
)

target_link_libraries(sound INTERFACE
        hardware_dma
        hardware_irq
        hardware_pio
	hardware_pwm
        pico_multicore
)

