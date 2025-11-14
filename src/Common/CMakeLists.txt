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
find_package(QT NAMES Qt6 Qt5 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
