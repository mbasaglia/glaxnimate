set(SOURCES

app_info.cpp

command/structure_commands.cpp
command/shape_commands.cpp
command/animation_commands.cpp

io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/glaxnimate/glaxnimate_mime.cpp
io/mime/mime_serializer.cpp
io/base.cpp

model/main_composition.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/object.cpp
model/transform.cpp
model/factory.cpp
model/animation_container.cpp
model/stretchable_time.cpp
model/comp_graph.cpp
model/mask_settings.cpp
model/visitor.cpp
model/custom_font.cpp

model/animation/keyframe_transition.cpp
model/animation/animatable.cpp
model/animation/animatable_path.cpp
model/property/property.cpp
model/property/reference_property.cpp
model/property/option_list_property.cpp

model/assets/assets.cpp
model/assets/brush_style.cpp
model/assets/named_color.cpp
model/assets/bitmap.cpp
model/assets/gradient.cpp
model/assets/asset_base.cpp
model/assets/asset.cpp
model/assets/precomposition.cpp
model/assets/embedded_font.cpp

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
model/shapes/precomp_layer.cpp
model/shapes/text.cpp
model/shapes/repeater.cpp
model/shapes/trim.cpp
model/shapes/inflate_deflate.cpp
model/shapes/path_modifier.cpp
model/shapes/round_corners.cpp

plugin/plugin.cpp
plugin/action.cpp
plugin/io.cpp
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)


add_library(glaxnimate_core OBJECT ${SOURCES})
set_property(TARGET glaxnimate_core APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_link_libraries(glaxnimate_core PUBLIC
    QtAppSetup
    glaxnimate_math
)
