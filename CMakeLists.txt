# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0077 NEW)


project(Glaxnimate VERSION 0.5.5 LANGUAGES CXX)
include(FeatureSummary)

set(PROJECT_SLUG "glaxnimate")
set(LIB_NAME_CORE "${PROJECT_SLUG}_core")
set(LIB_NAME_GUI "${PROJECT_SLUG}_gui")
set(LIB_NAME_PYTHON "${PROJECT_SLUG}_python_lib")
set(LIB_NAME_DESKTOP "${PROJECT_SLUG}_desktop")
set(DATA_INSTALL "share/${PROJECT_SLUG}/${PROJECT_SLUG}")
set(PROJECT_ID "org.mattbas.Glaxnimate")

set(PROJECT_HOMEPAGE_URL "https://glaxnimate.mattbas.org/")
set(URL_DOCS "https://glaxnimate.mattbas.org/")
set(URL_ISSUES "https://gitlab.com/mattbas/glaxnimate/-/issues")
set(URL_DONATE "https://glaxnimate.mattbas.org/donate/")

# You can use it to change the url used by `make docs`
set(DOCS_SITE_URL ${URL_DOCS} CACHE STRING "")
set(DOCS_REPO_URL "https://gitlab.com/mattbas/glaxnimate")
set(DOCS_CHAT_URL "https://t.me/Glaxnimate")

set(PROJECT_DESCRIPTION "Simple vector animation program")
set(PROJECT_MANINTAINER_NAME "Mattia Basaglia")
set(PROJECT_MANINTAINER_EMAIL "glanimate@mattbas.org")
set(PROJECT_MANINTAINER "${PROJECT_MANINTAINER_NAME} <${PROJECT_MANINTAINER_EMAIL}>")

option(MOBILE_UI "Build the mobile user interface" OFF)
option(INSTALL_TRANSLATIONS "Install translations" ON)

if (ANDROID)
    set(MOBILE_UI ON)
endif()

if(MOBILE_UI)
    message(STATUS "Using mobile UI")
    add_compile_definitions(MOBILE_UI)
else()
    message(STATUS "Using desktop UI")
endif()

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/external/cmake-modules")
include(misc)
include(GetGitRevisionDescription)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Git
set(VERSION_NAKED ${PROJECT_VERSION})
set(VERSION_SUFFIX "-git-" CACHE STRING "")

if (VERSION_SUFFIX STREQUAL "-git-")
    git_describe(GIT_DESC --tags)
    if ( NOT ${GIT_DESC} MATCHES ".*NOTFOUND" )
        set(PROJECT_VERSION ${GIT_DESC})
    endif()
elseif(VERSION_SUFFIX MATCHES "^\\..*")
    set(PROJECT_VERSION "${PROJECT_VERSION}${VERSION_SUFFIX}")
elseif(NOT VERSION_SUFFIX STREQUAL "")
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

# Fucking Windows being shit
if ( WIN32 AND MinGW )
    # Without this shit it gives a bunch of errors like this:
    # * referenced in section `.rdata' of * defined in discarded section `.gnu.linkonce.*`
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-lto")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-lto")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-lto")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(CMAKE_ENABLE_EXPORTS TRUE)
endif()


# Python
if ( NOT ANDROID )
    set(PYTHON_SCRIPTING_ENABLED ON)
else()
    set(PYTHON_SCRIPTING_ENABLED OFF)
endif()

if ( PYTHON_SCRIPTING_ENABLED )
    find_program(PYTHON_PIP pip3)
    if(NOT PYTHON_PIP)
        find_program(PYTHON_PIP pip)
    endif()

    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS ${Python3_EXECUTABLE})
else()
    message(STATUS "Python Disabled")
endif()

# QT_MAJOR_VERSION is the common name in KDE apps and infrastructure.
# However some third-party submodules use QT_VERSION_MAJOR so we need to set that too.
if (NOT DEFINED QT_MAJOR_VERSION)
    set(QT_MAJOR_VERSION 5)
endif()
set(QT_VERSION_MAJOR ${QT_MAJOR_VERSION})

# Qt
find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Core Widgets Xml UiTools Concurrent Network Svg REQUIRED)
message(STATUS "Using Qt ${QT_MAJOR_VERSION}")

# KF
set(KF_DEP_VERSION "5.92.0")
find_package(ECM ${KF_DEP_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

# TODO: The following definitons should be enabled
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

if (NOT DEFINED KF_MAJOR)
  set(KF_MAJOR ${QT_MAJOR_VERSION})
endif()

if (KF_MAJOR STREQUAL "6")
  set(KF_DEP_VERSION "5.240.0")
endif()

find_package(KF${KF_MAJOR} ${KF_DEP_VERSION}
    REQUIRED COMPONENTS
    CoreAddons
    WidgetsAddons
    XmlGui
    Archive
    Completion
)
if ( NOT ANDROID )
    find_package(KF${KF_MAJOR} ${KF_DEP_VERSION}
        REQUIRED COMPONENTS
        Crash
    )
endif()


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Potrace
option(GLAXNIMATE_SYSTEM_POTRACE "Use the system potrace libarary" ON)
if (NOT GLAXNIMATE_SYSTEM_POTRACE)
    message(STATUS "Using bundled potrace")
    set(CMAKE_C_COMPILE_OBJECT ${CMAKE_CXX_COMPILE_OBJECT})
    add_subdirectory(external/potrace/)
    set(Potrace_LIBRARIES potrace)
    include_directories(external/potrace/src/)
else()
    find_package(Potrace REQUIRED)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra")
message(STATUS "${CMAKE_BUILD_TYPE} build")

# sub projects and stuff

SET(BUILD_STATIC_LIBS ON CACHE BOOL "")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")

add_subdirectory(external/QtAppSetup)

add_subdirectory(src bin)

set(QTCOLORWIDGETS_DESIGNER_PLUGIN ON)
add_subdirectory(external/Qt-Color-Widgets EXCLUDE_FROM_ALL)

set(ALL_SOURCE_DIRECTORIES src)
find_sources(ALL_SOURCES *.cpp *.hpp)

add_subdirectory(data ${DATA_INSTALL})

if(BUILD_TESTING)
    find_package(Qt${QT_MAJOR_VERSION}Test REQUIRED)
    message(STATUS "Tests enabled")
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
    release_0
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_0.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    release_1
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_1.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    release_check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_check.py ${VERSION_NAKED}
)

# Misc files
if (ANDROID)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/android/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/deploy/org.mattbas.Glaxnimate.metainfo.xml DESTINATION share/metainfo)

# Packaging
# -- see https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators
set(CPACK_PACKAGE_CONTACT ${PROJECT_MANINTAINER})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

execute_process(COMMAND dpkg-architecture -qDEB_BUILD_ARCH OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5xml5 (>= 5.10.0), libqt5widgets5 (>= 5.10.0), libpython3.10 (>= 3.10.0), zlib1g (>= 1:1.2.11), libpotrace0 (>= 1.16-2), libavformat58, libswscale5, qt5-image-formats-plugins, libarchive13")
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

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
