set(SOURCES
svg_renderer.cpp
svg_parser.cpp
detail.cpp
svg_format.cpp
)

add_library(glaxnimate_svg OBJECT ${SOURCES})
target_link_libraries(glaxnimate_svg PUBLIC
    glaxnimate_compress
    glaxnimate_core
    Qt${QT_VERSION_MAJOR}::Xml
)
