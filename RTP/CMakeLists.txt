cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

project(RTP)

add_library(RTP INTERFACE)

target_include_directories(RTP INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(RTP INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/RTPPacket.c
)

target_link_libraries(RTP INTERFACE
        pico_multicore
)

