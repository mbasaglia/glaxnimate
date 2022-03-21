include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(glaxnimate)

if ( ANDROID )
    add_subdirectory(android)
else()
    add_subdirectory(python)
    add_executable(${PROJECT_SLUG} WIN32)
    target_link_libraries(${PROJECT_SLUG}
        glaxnimate_core
        glaxnimate_gui
        glaxnimate_trace
        glaxnimate_video
        glaxnimate_lottie
        glaxnimate_compress
        glaxnimate_math
        glaxnimate_raster
        glaxnimate_svg
        ${LIB_NAME_PYTHON}
        QtAppSetup
    )
    install(TARGETS ${PROJECT_SLUG} DESTINATION bin)
endif()
