cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(PICO_BOARD pico_w)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(voip 
            voip.c
	    settings.c	
	    md5.c
            sip_client.c 
            sip_parse.c
	    iniparser.c
            hw_config.c
            device_sdfs.c
            dictionary.c
)

pico_enable_stdio_usb(voip 1)
#pico_enable_stdio_uart(voip 1)

pico_add_extra_outputs(voip)
target_include_directories(voip PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(voip 
		sound
                pico_cyw43_arch_lwip_threadsafe_background 
                pico_stdlib
		pico_lwip_mbedtls
		pico_multicore
		hardware_spi
	        FatFs_SPI
        	hardware_adc
        	hardware_pwm
        	pico_rand
		display
		keyboard
		g711
		RTP
)
add_subdirectory(FatFs_SPI build)
add_subdirectory(sound)
add_subdirectory(display)
add_subdirectory(keyboard)
add_subdirectory(g711)
add_subdirectory(RTP)

#auto reboot/program
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)

