
cmake_minimum_required (VERSION 3.7 FATAL_ERROR)

project(Glaxnimate VERSION 0.0.1 DESCRIPTION "Simple vector animation program" LANGUAGES CXX)
set(PROJECT_SLUG "glaxnimate")


# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(misc)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set (QT_SUPPORTED_VERSIONS 5)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Other libs

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


set(ALL_SOURCE_DIRECTORIES src)
find_sources(ALL_SOURCES *.cpp *.hpp)

add_subdirectory(data share/${PROJECT_SLUG}/${PROJECT_SLUG})

find_package(Qt5Test QUIET)
if(Qt5Test_FOUND)
    add_subdirectory(test)
endif()

# Doxygen

set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(docs)
endif(DOXYGEN_FOUND)
