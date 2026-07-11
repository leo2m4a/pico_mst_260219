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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_mst_260219 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add RadioLib as a subdirectory
add_subdirectory(RadioLib usr_lib )

# Add executable. Default name is the project name, version 0.1

add_executable(pico_mst_260219 pico_mst_260219.cpp
RadioLib/src/hal/RPiPico/PicoHal.cpp
    RadioLib/src/modules/SX126x/SX126x.cpp
    RadioLib/src/modules/SX126x/SX1262.cpp
    usr_lib/i2c_lcd.cpp
    usr_lib/at24c32.c
    mst_ur_proc.cpp
    mst_rf_cmd_proc.cpp
    )


pico_set_program_name(pico_mst_260219 "pico_mst_260219")
pico_set_program_version(pico_mst_260219 "0.1")

# 定義 RADIOLIB_BUILD_RPI_PICO 以啟用 PicoHal
target_compile_definitions(pico_mst_260219 PRIVATE RADIOLIB_BUILD_RPI_PICO )

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico_mst_260219 0)
pico_enable_stdio_usb(pico_mst_260219 1)

# Add the standard library to the build
target_link_libraries(pico_mst_260219
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(pico_mst_260219 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/RadioLib/src
        ${CMAKE_CURRENT_LIST_DIR}/RadioLib/src/hal/RPiPico
        ${CMAKE_CURRENT_LIST_DIR}/usr_lib
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(pico_mst_260219 
        hardware_spi
        hardware_i2c
        hardware_timer
        hardware_uart
        RadioLib
        hardware_pwm
        pico_multicore    
        )

pico_add_extra_outputs(pico_mst_260219)

