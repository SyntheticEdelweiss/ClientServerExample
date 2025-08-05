cmake_minimum_required(VERSION 3.16)
project(Server VERSION 0.1 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../global.cmake)

set(PROJECT_SOURCES
    main.cpp
    ExampleServer.hpp ExampleServer.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

find_package(Qt5 REQUIRED COMPONENTS Core)
target_link_directories(${PROJECT_NAME} PRIVATE
    # ../
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Net
    Qt5::Core
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
