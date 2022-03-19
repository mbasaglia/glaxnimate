set(SOURCES
raster_format.cpp
spritesheet_format.cpp
)

add_library(glaxnimate_raster OBJECT ${SOURCES})
target_link_libraries(glaxnimate_raster PUBLIC
    glaxnimate_trace
)
