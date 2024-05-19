cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

project(display)

add_library(display INTERFACE)

target_include_directories(display INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(display INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/display.c
)

target_link_libraries(display INTERFACE
        hardware_dma
        hardware_irq
        hardware_i2c
        pico_multicore
)

