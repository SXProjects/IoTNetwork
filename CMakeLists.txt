cmake_minimum_required(VERSION 3.21)

project(SmartNetwork)
set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)

ExternalProject_Add(vcpkg
        GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
        SOURCE_DIR=${CMAKE_BINARY_DIR}
        )

find_package(websocketpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(SmartNetwork
        src/SmartNetwork/Capabilities.cpp
        src/SmartNetwork/ServerCommands.cpp
        src/SmartNetwork/DeviceMap.cpp
        src/SmartNetwork/WebsocketServer.cpp
        src/SmartNetwork/DeviceRelations.cpp)
target_include_directories(SmartNetwork PUBLIC src)
target_link_libraries(SmartNetwork PUBLIC websocketpp::websocketpp nlohmann_json::nlohmann_json)

add_executable(SmartNetworkRun app/main.cpp)
target_link_libraries(SmartNetworkRun PUBLIC SmartNetwork)

add_executable(Test app/test.cpp)
target_link_libraries(Test PUBLIC SmartNetwork)
