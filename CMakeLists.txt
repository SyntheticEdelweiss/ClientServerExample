cmake_minimum_required(VERSION 3.16)
project(ClientServerExample VERSION 1.0)

include(GlobalConfig.cmake)

set(MESSAGE_FORMAT "JSON" CACHE STRING "Message format for client-server communication")
set_property(CACHE MESSAGE_FORMAT PROPERTY STRINGS BINARY JSON)

add_subdirectory(src/Common)
add_subdirectory(src/Net)
add_subdirectory(src/Server)
add_subdirectory(src/Client)

target_compile_definitions(Server PUBLIC "MESSAGE_FORMAT_${MESSAGE_FORMAT}")
target_compile_definitions(Client PUBLIC "MESSAGE_FORMAT_${MESSAGE_FORMAT}")
