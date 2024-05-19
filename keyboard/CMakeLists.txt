cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

project(keyboard)

add_library(keyboard INTERFACE)

target_include_directories(keyboard INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(keyboard INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/keyboard.c
)

target_link_libraries(keyboard INTERFACE
        hardware_irq
        pico_multicore
)

