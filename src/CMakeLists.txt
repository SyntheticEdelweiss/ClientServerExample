cmake_minimum_required(VERSION 3.16)
project(ClientServerExample VERSION 0.1 LANGUAGES CXX)

include(CMakePrintHelpers)
include(global.cmake)
get_filename_component(build_name ${CMAKE_BINARY_DIR} NAME)

add_compile_definitions(
    QT_NO_DEPRECATED_WARNINGS
)

add_subdirectory(Common)
add_subdirectory(Net)
add_subdirectory(Server)
add_subdirectory(Client)
