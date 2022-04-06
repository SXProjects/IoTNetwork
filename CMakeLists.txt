cmake_minimum_required(VERSION 3.21)

project(SmartNetwork)
set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)

ExternalProject_Add(vcpkg
        GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
        SOURCE_DIR=${CMAKE_BINARY_DIR}
        )
add_compile_options(-pthread)

find_package(websocketpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(Threads)

add_library(SmartNetwork
        src/SmartNetwork/Capabilities.cpp
        src/SmartNetwork/Commands.cpp
        src/SmartNetwork/DeviceMap.cpp
        src/SmartNetwork/Relations.cpp)
target_include_directories(SmartNetwork PUBLIC src)
target_link_libraries(SmartNetwork PUBLIC websocketpp::websocketpp
        nlohmann_json::nlohmann_json cereal::cereal ${CMAKE_THREAD_LIBS_INIT})

add_executable(SmartNetworkRun app/run.cpp)
target_link_libraries(SmartNetworkRun PUBLIC SmartNetwork ${CMAKE_THREAD_LIBS_INIT})
if (MSVC)
    target_compile_options(SmartNetworkRun PRIVATE /bigobj)
endif ()

add_executable(SmartNetworkTest app/test.cpp)
target_link_libraries(SmartNetworkTest PUBLIC SmartNetwork ${CMAKE_THREAD_LIBS_INIT})
if (MSVC)
    target_compile_options(SmartNetworkTest PRIVATE /bigobj)
endif ()
