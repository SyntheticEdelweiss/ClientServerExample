cmake_minimum_required(VERSION 3.16)
project(Net VERSION 1.0)

add_library(${PROJECT_NAME}
    NetConnection.cpp
    NetConnection.hpp
    NetHeaders.cpp
    NetHeaders.hpp
    NetThread.cpp
    NetThread.hpp
    NetUtils.cpp
    NetUtils.hpp
    TcpClient.cpp
    TcpClient.hpp
    TcpServer.cpp
    TcpServer.hpp
)

find_package(QT NAMES Qt5 Qt6 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Network
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32)
endif()

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

set(NET_ENDIANNESS "LittleEndian" CACHE STRING "Byte order for QDataStream (ByteOrder enum)")
set_property(CACHE NET_ENDIANNESS PROPERTY STRINGS LittleEndian BigEndian)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NET_ENDIANNESS=QDataStream::${NET_ENDIANNESS}
)
