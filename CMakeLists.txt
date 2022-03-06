cmake_minimum_required(VERSION 3.21)
project(SmartNetwork)

set(CMAKE_CXX_STANDARD 17)

add_library(SmartNetwork
        src/SmartNetwork/DeviceCapabilities.cpp
        src/SmartNetwork/DeviceConnection.cpp
        src/SmartNetwork/DeviceMap.cpp
        src/SmartNetwork/DeviceData.cpp)
target_include_directories(SmartNetwork PUBLIC src)

add_executable(SmartNetworkRun app/main.cpp)
target_link_libraries(SmartNetworkRun PUBLIC SmartNetwork)

add_executable(DeviceMapTest app/map.cpp)
target_link_libraries(DeviceMapTest PUBLIC SmartNetwork)
