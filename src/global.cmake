set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ClientServerExample_ROOT ${CMAKE_SOURCE_DIR}/..)
set(ClientServerExample_SRC ${ClientServerExample_ROOT}/src)
set(ClientServerExample_BIN ${ClientServerExample_ROOT}/bin)
set(ClientServerExample_LIB ${ClientServerExample_ROOT}/lib)
set(ClientServerExample_MOC ${ClientServerExample_ROOT}/moc)
set(ClientServerExample_OBJ ${ClientServerExample_ROOT}/obj)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ClientServerExample_BIN})
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ClientServerExample_LIB})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_CONFIGURATION_TYPES MATCHES "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    #
elseif(CMAKE_CONFIGURATION_TYPES MATCHES "Release" OR CMAKE_BUILD_TYPE STREQUAL "Release")
    # Adjust output directories for release
    # set(ClientServerExample_BIN "${ClientServerExample_BIN}.release")
    # set(ClientServerExample_LIB "${ClientServerExample_LIB}.release")
    # set(ClientServerExample_MOC "${ClientServerExample_MOC}.release")
    # set(ClientServerExample_OBJ "${ClientServerExample_OBJ}.release")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
add_definitions(-DUNICODE -D_UNICODE)
