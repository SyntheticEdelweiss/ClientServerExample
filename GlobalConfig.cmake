include_guard(DIRECTORY)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_DEBUG_POSTFIX "d")
option(BUILD_SHARED_LIBS "Build libraries as shared(ON) or static(OFF)" OFF)
# With UNICODE, TCHAR becomes wchar_t instead of char (not sure) and it's only used in lib Net so probably should be defined there as PUBLIC
add_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(
    QT_NO_DEPRECATED_WARNINGS
)

# All subdirs have to include GlobalConfig.cmake anyway, so more convenient to just find all Qt packages here
find_package(QT NAMES Qt5 Qt6 REQUIRED) # find Qt*Config.cmake and set QT_VERSION_MAJOR, etc.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    Widgets
    Charts
)