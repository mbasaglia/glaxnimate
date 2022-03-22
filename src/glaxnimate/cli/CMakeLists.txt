
add_executable(lottie_trace EXCLUDE_FROM_ALL lottie_trace.cpp)
target_link_libraries(lottie_trace PRIVATE
    glaxnimate_core
    glaxnimate_math
    glaxnimate_trace
    glaxnimate_lottie
    glaxnimate_raster
    glaxnimate_svg
    glaxnimate_compress
    Qt${QT_VERSION_MAJOR}::Gui
    QtAppSetup
)
