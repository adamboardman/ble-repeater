cmake_minimum_required(VERSION 3.12)

add_definitions(-DMOCK_PICO_PI)

project(tests)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

find_package(Catch2 REQUIRED)

include_directories(../include ../CircularBuffer)

add_executable(tests
        ../BLE/BleConnection.cpp
        ../BLE/BleConnectionTracker.cpp
        ../Packet/Base.cpp
        ../Packet/PacketBase.cpp
        ../Packet/Peer.cpp
        ../Packet/ProtocolProcessor.cpp
        ../Packet/BinaryReader.cpp
        ../Packet/BinaryWriter.cpp
        ../Packet/Message.cpp
        ../Packet/Announce.cpp
        ../Packet/PacketPassAlong.cpp
        pico_pi_mocks.cpp
        test_ble_smoke_detector.cpp
        test_circular_buffer.cpp
        test_packet.cpp
)

target_link_libraries(tests PRIVATE
        Catch2::Catch2WithMain
        libdeflate_static
)

include(CTest)
include(Catch)
catch_discover_tests(tests)

add_subdirectory(../modules/libdeflate libdeflate)