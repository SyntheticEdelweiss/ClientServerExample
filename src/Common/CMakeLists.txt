cmake_minimum_required(VERSION 3.16)
project(Common VERSION 1.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../global.cmake)

set(PROJECT_SOURCES
    RegLogger.hpp RegLogger.cpp
    Protocol.hpp Protocol.cpp
    Utils.hpp Utils.cpp
)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES}
    )
find_package(Qt5 REQUIRED COMPONENTS Core)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt5::Core
)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
