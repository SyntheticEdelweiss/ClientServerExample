cmake_minimum_required(VERSION 3.16)
project(Client VERSION 1.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../global.cmake)

set(PROJECT_SOURCES
    main.cpp    
    MainWindow.hpp MainWindow.cpp MainWindow.ui
    AppSettings.hpp AppSettings.cpp AppSettings.ui
    PersistentProgressDialog.hpp PersistentProgressDialog.cpp
    # Client.qrc
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})


find_package(QT NAMES Qt6 Qt5 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Charts)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Net
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
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
