cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
# cmake_policy(SET CMP0076 NEW)


project(Glaxnimate VERSION 0.0.1 LANGUAGES CXX)
set(PROJECT_SLUG "glaxnimate")
set(LIB_NAME "${PROJECT_SLUG}_core")
set(LIB_NAME_GUI "${PROJECT_SLUG}_gui")
set(URL_DOCS "https://glaxnimate.mattbas.org/")
set(URL_ISSUES "https://gitlab.com/mattia.basaglia/glaxnimate/-/issues")
set(DATA_INSTALL "share/${PROJECT_SLUG}/${PROJECT_SLUG}")
set(PROJECT_DESCRIPTION "Simple vector animation program")

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
add_subdirectory(external/QtAppSetup)
add_subdirectory(src bin)

SET(BUILD_STATIC_LIBS ON CACHE BOOL "")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(external/Qt-Color-Widgets EXCLUDE_FROM_ALL)


set(ALL_SOURCE_DIRECTORIES src)
find_sources(ALL_SOURCES *.cpp *.hpp)

add_subdirectory(data ${DATA_INSTALL})

find_package(Qt5Test QUIET)
if(Qt5Test_FOUND)
    add_subdirectory(test)
endif()

add_subdirectory(docs)

# Doxygen

set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(doxygen)
endif(DOXYGEN_FOUND)


# AppImage
add_custom_target(
    AppDir_base
    COMMAND make install DESTDIR=AppDir
    COMMAND cp AppDir/usr/${DATA_INSTALL}/${PROJECT_SLUG}.desktop AppDir
#     COMMAND cp AppDir/usr/${DATA_INSTALL}/images/logo.svg AppDir/${PROJECT_SLUG}.svg
    # COMMAND convert AppDir/usr/${DATA_INSTALL}/images/logo.svg -resize 256x256 AppDir/${PROJECT_SLUG}.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PROJECT_SLUG} translations
)
add_custom_target(
    AppDir_clean
    COMMAND rm -rf AppDir
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/linuxdeploy-x86_64.AppImage
    COMMAND curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage >linuxdeploy-x86_64.AppImage
    COMMAND chmod a+x linuxdeploy-x86_64.AppImage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)
add_custom_target(
    AppDir
    COMMAND ./linuxdeploy-x86_64.AppImage --appdir AppDir --desktop-file=AppDir/${PROJECT_SLUG}.desktop
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS AppDir_base ${CMAKE_CURRENT_BINARY_DIR}/linuxdeploy-x86_64.AppImage
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appimagetool-x86_64.AppImage
    COMMAND curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage >appimagetool-x86_64.AppImage
    COMMAND chmod a+x appimagetool-x86_64.AppImage
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)
add_custom_target(
    AppImage
    COMMAND ./appimagetool-x86_64.AppImage AppDir
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS AppDir ${CMAKE_CURRENT_BINARY_DIR}/appimagetool-x86_64.AppImage
)
