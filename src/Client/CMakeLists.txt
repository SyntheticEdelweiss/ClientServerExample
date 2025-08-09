cmake_minimum_required(VERSION 3.16)
project(Client VERSION 0.1 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../global.cmake)

set(PROJECT_SOURCES
    main.cpp    
    MainWindow.hpp MainWindow.cpp MainWindow.ui
    AppSettings.hpp AppSettings.cpp AppSettings.ui
    PersistentProgressDialog.hpp PersistentProgressDialog.cpp
    # Client.qrc
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Charts)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Net
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Charts
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE # FALSE keeps console output
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
