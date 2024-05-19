cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

project(g711)

add_library(g711 INTERFACE)

target_include_directories(g711 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(g711 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/g711.c
)

target_link_libraries(g711 INTERFACE
        pico_multicore
)

