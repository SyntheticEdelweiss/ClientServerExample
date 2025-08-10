cmake_minimum_required(VERSION 3.16)
project(ClientServerExample VERSION 0.1 LANGUAGES CXX)

include(CMakePrintHelpers)
include(global.cmake)
get_filename_component(build_name ${CMAKE_BINARY_DIR} NAME)

add_compile_definitions(
    QT_NO_DEPRECATED_WARNINGS
)

set(MESSAGE_FORMAT "JSON"
    CACHE STRING "Message format for client-server communication"
)
set_property(
    CACHE MESSAGE_FORMAT
    PROPERTY STRINGS BINARY JSON
)

add_subdirectory(Common)
add_subdirectory(Net)
add_subdirectory(Server)
add_subdirectory(Client)

if(MESSAGE_FORMAT STREQUAL "JSON")
    target_compile_definitions(Server PUBLIC MESSAGE_FORMAT_JSON)
    target_compile_definitions(Client PUBLIC MESSAGE_FORMAT_JSON)
elseif(MESSAGE_FORMAT STREQUAL "BINARY")
    target_compile_definitions(Server PUBLIC MESSAGE_FORMAT_BINARY)
    target_compile_definitions(Client PUBLIC MESSAGE_FORMAT_BINARY)
endif()
