cmake_minimum_required(VERSION 3.16)
project(NetUIQt LANGUAGES CXX)

# set(SQLITE3_DIR "./libs")

set(CMAKE_CXX_STANDARD 17)

# MinGW specific settings
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition -Wl,--undefined=__imp___argc")
endif()

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64/lib/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)  # Add Network here

add_executable(IPGui ng.cpp appicon.rc)

target_link_libraries(IPGui PRIVATE Qt6::Widgets Qt6::Network)  # Link Network

# Temporarily disable WIN32_EXECUTABLE to avoid Qt6EntryPoint issues with MinGW
# set_target_properties(IPGui PROPERTIES WIN32_EXECUTABLE TRUE)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)

target_link_libraries(IPGui PRIVATE 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Concurrent
    iphlpapi 
    ws2_32 
    netapi32 
    wlanapi 
    crypt32
)

# find_package(SQLite3 REQUIRED)
# include_directories(${SQLite3_INCLUDE_DIRS})
# target_link_libraries(IPGui PRIVATE SQLite3::SQLite3)
