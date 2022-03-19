set(SOURCES
video_format.cpp
)

find_package(Libav COMPONENTS codec util format swscale REQUIRED)
include_directories(${Libav_INCLUDE_DIRS})

add_library(glaxnimate_video OBJECT ${SOURCES})
target_link_libraries(glaxnimate_video PUBLIC glaxnimate_core ${Libav_LIBRARIES})
