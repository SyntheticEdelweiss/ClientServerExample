cmake_minimum_required(VERSION 3.16)
project(Client VERSION 1.0)

add_executable(${PROJECT_NAME}
    AppSettings.cpp
    AppSettings.hpp
    AppSettings.ui
    main.cpp
    MainWindow.cpp
    MainWindow.hpp
    MainWindow.ui
    PersistentProgressDialog.cpp
    PersistentProgressDialog.hpp
)

find_package(QT NAMES Qt5 Qt6 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Charts
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Net
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
)
