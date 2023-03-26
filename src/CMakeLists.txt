# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gui/)

add_subdirectory(core)
add_subdirectory(gui)
if ( ANDROID )
    add_subdirectory(android)
else()
    add_subdirectory(python)
    add_executable(${PROJECT_SLUG} WIN32)
    target_link_libraries(${PROJECT_SLUG} ${LIB_NAME_CORE} ${LIB_NAME_GUI} ${LIB_NAME_PYTHON} QtAppSetup)
    install(TARGETS ${PROJECT_SLUG} DESTINATION bin)
endif()
