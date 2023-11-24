# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause
cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0076 NEW)

project(QtAppSetup VERSION 0.1.0 LANGUAGES CXX)
set(PROJECT_SLUG QtAppSetup)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt${QT_VERSION_MAJOR}Qml QUIET)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Submodules
add_subdirectory(src)

option(SCRIPTING_JS "Enable JS scripting" OFF)
if(Qt5Qml_FOUND AND SCRIPTING_JS)
    message(STATUS "JS enabled")
    add_subdirectory(src/app/scripting/js/)
endif()


option(SCRIPTING_PY "Enable Python scripting" OFF)
find_package(Python3 COMPONENTS Interpreter Development QUIET)
if(Python3_FOUND AND SCRIPTING_PY)
    set(PYTHON_ENABLED ON PARENT_SCOPE)
    message(STATUS "Python3 enabled")
    add_subdirectory(external/pybind11)
    add_subdirectory(src/app/scripting/python)
    target_include_directories (${PROJECT_SLUG} PUBLIC
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/pybind11/include
    )
else()
    set(PYTHON_ENABLED OFF PARENT_SCOPE)
endif()
