set(SOURCES
quantize.cpp
trace.cpp
trace_wrapper.cpp
segmentation.cpp
color.cpp
gradient.cpp
)

if ( NOT ANDROID )
    include_directories(${Potrace_INCLUDE_DIRS})
endif()

add_library(glaxnimate_trace OBJECT ${SOURCES})
target_link_libraries(glaxnimate_trace PUBLIC glaxnimate_math glaxnimate_core ${Potrace_LIBRARIES})
