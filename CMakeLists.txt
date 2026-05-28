cmake_minimum_required(VERSION 3.13)

if ("rp2350-arm-s" STREQUAL PICO_PLATFORM)
    set(PICO_BOARD pico2_w)  # needed for Pico2-W, otherwise #include "pico/cyw43_arch.h" won't be found
else ()
    set(PICO_BOARD pico_w)  # needed for Pico-W, otherwise #include "pico/cyw43_arch.h" won't be found
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF)
set(LIBDEFLATE_BUILD_GZIP OFF)

set(PICO_USE_STACK_GUARDS 1)

include(pico_sdk_import.cmake)

project(ble_repeater C CXX ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

add_definitions(-DPICO_ENTER_USB_BOOT_ON_EXIT)

pico_sdk_init()

add_executable(ble_repeater main.cpp
        BLE/BleConnection.cpp
        BLE/BleConnectionTracker.cpp
        CircularBuffer/Debugging.cpp
        Packet/Peer.cpp
        Packet/Base.cpp
        Packet/PacketBase.cpp
        Packet/Announce.cpp
        Packet/Message.cpp
        Packet/ProtocolProcessor.cpp
        Packet/BinaryReader.cpp
        Packet/BinaryWriter.cpp
        Packet/PacketPassAlong.cpp
)

include_directories(include CircularBuffer)

pico_enable_stdio_usb(ble_repeater 1) # Use this when debugging from a uf2 usb dropped build
pico_enable_stdio_uart(ble_repeater 1) # Use this when debugging through the debugger

target_link_libraries(ble_repeater
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_none
        hardware_gpio
        hardware_adc
        hardware_pwm
        libdeflate_static
)

target_include_directories(ble_repeater PRIVATE
        ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
pico_btstack_make_gatt_header(ble_repeater PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_repeater.gatt")

pico_add_extra_outputs(ble_repeater)

add_subdirectory(modules/libdeflate)