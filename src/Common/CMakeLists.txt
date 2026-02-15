cmake_minimum_required(VERSION 3.16)
project(Common VERSION 1.0)

add_library(${PROJECT_NAME}
    Protocol.cpp
    Protocol.hpp
    RegLogger.cpp
    RegLogger.hpp
    Utils.cpp
    Utils.hpp
)

find_package(QT NAMES Qt5 Qt6 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
