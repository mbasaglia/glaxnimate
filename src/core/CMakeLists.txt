set(SOURCES

app_info.cpp

command/structure_commands.cpp
command/shape_commands.cpp
command/animation_commands.cpp

io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/lottie/tgs_format.cpp
io/lottie/cbor_write_json.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/glaxnimate/glaxnimate_mime.cpp
io/svg/svg_renderer.cpp
io/svg/svg_parser.cpp
io/svg/detail.cpp
io/svg/svg_format.cpp
io/mime/mime_serializer.cpp
io/raster/raster_format.cpp
io/base.cpp

math/geom.cpp
math/bezier/bezier.cpp
math/bezier/point.cpp
math/bezier/operations.cpp
math/bezier/cubic_struts.cpp

model/main_composition.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/object.cpp
model/transform.cpp
model/factory.cpp
model/reference_target.cpp
model/animation_container.cpp

model/animation/keyframe_transition.cpp
model/animation/animatable.cpp
model/property/property.cpp
model/property/reference_property.cpp

model/defs/defs.cpp
model/defs/brush_style.cpp
model/defs/named_color.cpp
model/defs/bitmap.cpp
model/defs/gradient.cpp
model/defs/asset.cpp

model/shapes/shape.cpp
model/shapes/fill.cpp
model/shapes/rect.cpp
model/shapes/group.cpp
model/shapes/ellipse.cpp
model/shapes/path.cpp
model/shapes/stroke.cpp
model/shapes/polystar.cpp
model/shapes/styler.cpp
model/shapes/layer.cpp
model/shapes/image.cpp

utils/gzip.cpp
)

if(${PYTHON_ENABLED})
    list(APPEND SOURCES python.cpp)
endif()

find_package(ZLIB REQUIRED)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)

add_library(${LIB_NAME_CORE} OBJECT ${SOURCES})
target_link_libraries(${LIB_NAME_CORE} PUBLIC QtAppSetup ZLIB::ZLIB Qt5::Xml)
set_property(TARGET ${LIB_NAME_CORE} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_include_directories(${LIB_NAME_CORE} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
