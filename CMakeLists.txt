cmake_minimum_required(VERSION 3.16)
project(NetUIQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64/lib/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)  # Add Network here

add_executable(IPGui ng.cpp appicon.rc)

target_link_libraries(IPGui PRIVATE Qt6::Widgets Qt6::Network)  # Link Network


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

target_link_libraries(IPGui PRIVATE iphlpapi ws2_32)
