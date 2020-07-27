cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0076 NEW)


project(Glaxnimate VERSION 0.0.1 DESCRIPTION "Simple vector animation program" LANGUAGES CXX)
set(PROJECT_SLUG "glaxnimate")


# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/external/cmake-modules")
include(misc)
include(GetGitRevisionDescription)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set (QT_SUPPORTED_VERSIONS 5)
find_package(Qt5Qml QUIET)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Git
git_describe(GIT_DESC --tags)
if ( NOT ${GIT_DESC} MATCHES ".*NOTFOUND" )
    set(PROJECT_VERSION ${GIT_DESC})
endif()

message(STATUS ${PROJECT_VERSION})

# Debug flags
if(CMAKE_BUILD_TYPE EQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra")

# sub projects and stuff
add_subdirectory(src bin)

SET(BUILD_STATIC_LIBS ON CACHE BOOL "")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(external/Qt-Color-Widgets)
add_subdirectory(external/Qt-History-LineEdit)


set(ALL_SOURCE_DIRECTORIES src)
find_sources(ALL_SOURCES *.cpp *.hpp)

add_subdirectory(data share/${PROJECT_SLUG}/${PROJECT_SLUG})

find_package(Qt5Test QUIET)
if(Qt5Test_FOUND)
    add_subdirectory(test)
endif()

if(Qt5Qml_FOUND)
    message(STATUS "JS enabled")
    add_subdirectory(src/scripting/js/)
endif()


find_package(PythonLibs 3 COMPONENTS Interpreter Development QUIET)
if(PythonLibs_FOUND)
    message(STATUS "Python3 enabled")
    add_subdirectory(external/pybind11)
    add_subdirectory(src/scripting/python)
endif()

# Doxygen

set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(docs)
endif(DOXYGEN_FOUND)
