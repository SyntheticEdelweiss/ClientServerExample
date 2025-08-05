cmake_minimum_required(VERSION 3.16)
project(Net VERSION 1.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../global.cmake)




set(PROJECT_SOURCES    
    NetConnection.hpp NetConnection.cpp
    NetHeaders.hpp NetHeaders.cpp
    NetThread.hpp NetThread.cpp
    NetUtils.hpp NetUtils.cpp
    TcpClient.hpp TcpClient.cpp
    TcpServer.hpp TcpServer.cpp
)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
find_package(Qt5 REQUIRED COMPONENTS Core Network)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt5::Core
    Qt5::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        wsock32
        ws2_32
    )
endif()

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

set(NET_ENDIANNESS "LittleEndian"
    CACHE STRING "Byte order for QDataStream (ByteOrder enum)"
)
set_property(
    CACHE NET_ENDIANNESS
    PROPERTY STRINGS LittleEndian BigEndian
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NET_ENDIANNESS=QDataStream::${NET_ENDIANNESS}
)
