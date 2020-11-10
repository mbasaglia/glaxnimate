cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
# cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0077 NEW)


project(Glaxnimate VERSION 0.3.0 LANGUAGES CXX)
set(PROJECT_SLUG "glaxnimate")
set(LIB_NAME_CORE "${PROJECT_SLUG}_core")
set(LIB_NAME_GUI "${PROJECT_SLUG}_gui")
set(DATA_INSTALL "share/${PROJECT_SLUG}/${PROJECT_SLUG}")

set(PROJECT_HOMEPAGE_URL "https://glaxnimate.mattbas.org/")
set(URL_DOCS "https://glaxnimate.mattbas.org/")
set(URL_ISSUES "https://gitlab.com/mattbas/glaxnimate/-/issues")
set(URL_DONATE "https://glaxnimate.mattbas.org/donate/")

set(PROJECT_DESCRIPTION "Simple vector animation program")
set(PROJECT_MANINTAINER_NAME "Mattia Basaglia")
set(PROJECT_MANINTAINER_EMAIL "glanimate@mattbas.org")
set(PROJECT_MANINTAINER "${PROJECT_MANINTAINER_NAME} <${PROJECT_MANINTAINER_EMAIL}>")

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/external/cmake-modules")
include(misc)
include(GetGitRevisionDescription)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Git
set(VERSION_SUFFIX "" CACHE STRING "")
if (VERSION_SUFFIX STREQUAL "")
    git_describe(GIT_DESC --tags)
    if ( NOT ${GIT_DESC} MATCHES ".*NOTFOUND" )
        set(PROJECT_VERSION ${GIT_DESC})
    endif()
else()
    set(PROJECT_VERSION "${PROJECT_VERSION}+${VERSION_SUFFIX}")
endif()

message(STATUS "Building ${PROJECT_NAME} ${PROJECT_VERSION}")

# Snapcraft stuff here because we don't care if cmake fails on CI
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/deploy/snapcraft.in.yaml"
    "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
)
option(SNAP_IS_SUCH_A_PAIN "" OFF)
if ( SNAP_IS_SUCH_A_PAIN  )
    return()
endif()

# Python
find_program(PYTHON_PIP pip3)
if(NOT PYTHON_PIP)
    find_program(PYTHON_PIP pip)
endif()

find_program(PYTHON_EXEC python3)
if(NOT PYTHON_EXEC)
    find_program(PYTHON_EXEC python)
endif()

# Qt
set (QT_SUPPORTED_VERSIONS 5)
find_package(Qt5Qml QUIET)
find_package(Qt5 COMPONENTS Widgets Xml UiTools REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Potrace
find_package(Potrace REQUIRED)

# Debug flags
if(CMAKE_BUILD_TYPE EQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra")

# sub projects and stuff
set(SCRIPTING_PY ON)

if ( WIN32 )
    SET(BUILD_STATIC_LIBS OFF CACHE BOOL "")
    SET(BUILD_SHARED_LIBS ON CACHE BOOL "")
else ()
    SET(BUILD_STATIC_LIBS ON CACHE BOOL "")
    SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")
endif()

add_subdirectory(external/QtAppSetup)

add_subdirectory(external/Qt-History-LineEdit)

add_subdirectory(src bin)

set(QTCOLORWIDGETS_DESIGNER_PLUGIN ON)
add_subdirectory(external/Qt-Color-Widgets EXCLUDE_FROM_ALL)

add_subdirectory(py_module EXCLUDE_FROM_ALL)

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

# Release

add_custom_target(
    pre_release
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/pre_release.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    release
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    post_release
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_check.py ${PROJECT_VERSION}
)

# Packaging
# -- see https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators
set(CPACK_PACKAGE_CONTACT ${PROJECT_MANINTAINER})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

execute_process(COMMAND dpkg-architecture -qDEB_BUILD_ARCH OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5xml5 (>= 5.10.0), libqt5widgets5 (>= 5.10.0), libpython3.8 (>= 3.8.0), zlib1g (>= 1:1.2.11), libpotrace0 (1.16-2)")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})

set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/deploy/bundle.xml.plist"
    "${CMAKE_CURRENT_BINARY_DIR}/bundle.xml.plist"
)
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/bundle.xml.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.icns")

include(CPack)
