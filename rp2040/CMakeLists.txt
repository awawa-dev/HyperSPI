set(CMAKE_SYSTEM_NAME Generic)
# User configuration section starts here

# Some boards, such as the first Adafruit revisions, may have trouble booting properly
# due to bad componets used in the design.
# Turn this setting to ON if your rp2040 is not detected after firmware upload and reset
set(BOOT_WORKAROUND ON)

# Default output data pin for the non-SPI LED strips (only for sk6812/ws2812b)
set(OUTPUT_DATA_PIN 14)

# Use multi-segment, starting index of second led strip or OFF to disable
set(SECOND_SEGMENT_INDEX OFF)

# If multi-segment is used and it's reversed, set this option to ON to enable reversing
set(SECOND_SEGMENT_REVERSED OFF)

# HyperSPI communication interface with Raspberry Pi
set(OUTPUT_SPI_DATA_PIN 4)
set(OUTPUT_SPI_CLOCK_PIN 2)
set(OUTPUT_SPI_CHIP_SELECT 5)
set(OUTPUT_SPI_INTERFACE spi0)

# User configuration section ends here
# Usually you don't need to change anything below this section
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/HyperSerialPico/sdk/pico)
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/HyperSerialPico/sdk/freertos)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(HyperSPI_Pico C CXX ASM)
pico_sdk_init()

set(PICO_PROGRAM_MAIN_ENTRY "../hyperspi.cpp")
set(DISABLE_SPI_LEDS ON)
add_definitions(-DSPI_INTERFACE=${OUTPUT_SPI_INTERFACE}
				-DSPI_DATA_PIN=${OUTPUT_SPI_DATA_PIN}
				-DSPI_CLOCK_PIN=${OUTPUT_SPI_CLOCK_PIN}
				-DSPI_CHIP_SELECT=${OUTPUT_SPI_CHIP_SELECT})
add_subdirectory(HyperSerialPico) 
