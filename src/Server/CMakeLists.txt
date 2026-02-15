cmake_minimum_required(VERSION 3.16)
project(Server VERSION 1.0)

add_executable(${PROJECT_NAME}
    ExampleServer.cpp
    ExampleServer.hpp
    main.cpp
)

find_package(QT NAMES Qt5 Qt6 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Concurrent
    Core
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Net
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Core
)
