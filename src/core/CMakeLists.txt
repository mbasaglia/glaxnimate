# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES

app_info.cpp

command/structure_commands.cpp
command/shape_commands.cpp
command/animation_commands.cpp

io/base.cpp
io/binary_stream.cpp
io/utils.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/glaxnimate/glaxnimate_mime.cpp
io/lottie/cbor_write_json.cpp
io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/lottie/tgs_format.cpp
io/lottie/validation.cpp
io/mime/mime_serializer.cpp
io/raster/raster_format.cpp
io/raster/spritesheet_format.cpp
io/rive/rive_format.cpp
io/rive/rive_html_format.cpp
io/rive/rive_loader.cpp
io/rive/rive_serializer.cpp
io/rive/type_def.cpp
io/rive/type_system.cpp
io/svg/detail.cpp
io/svg/svg_format.cpp
io/svg/svg_parser.cpp
io/svg/svg_renderer.cpp
io/avd/avd_parser.cpp
io/avd/avd_format.cpp
io/avd/avd_renderer.cpp
io/aep/aep_format.cpp
io/aep/aep_loader.cpp
io/aep/string_decoder.cpp
io/aep/gradient_xml.cpp

math/geom.cpp
math/polynomial.cpp
math/ellipse_solver.cpp
math/bezier/bezier.cpp
math/bezier/point.cpp
math/bezier/operations.cpp
math/bezier/cubic_struts.cpp
math/bezier/meta.cpp
math/bezier/bezier_length.cpp

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
model/assets/composition.cpp
model/assets/embedded_font.cpp
model/assets/network_downloader.cpp

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
model/shapes/offset_path.cpp
model/shapes/zig_zag.cpp

plugin/plugin.cpp
plugin/action.cpp
plugin/io.cpp

utils/gzip.cpp
utils/quantize.cpp
utils/trace.cpp
utils/trace_wrapper.cpp
)

if ( NOT ANDROID )
    list(APPEND SOURCES
        io/video/video_format.cpp
        utils/tar.cpp
    )
    find_package(Libav COMPONENTS codec util format swscale REQUIRED)
    include_directories(${Libav_INCLUDE_DIRS})

    find_package(LibArchive REQUIRED)
    include_directories(${LibArchive_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)

if ( GLAXNIMATE_SYSTEM_POTRACE )
    include_directories(${Potrace_INCLUDE_DIRS})
endif()

add_library(${LIB_NAME_CORE} OBJECT ${SOURCES})
target_link_libraries(${LIB_NAME_CORE} PUBLIC QtAppSetup ZLIB::ZLIB Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Network ${Potrace_LIBRARIES})
set_property(TARGET ${LIB_NAME_CORE} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_include_directories(${LIB_NAME_CORE} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if ( NOT ANDROID )
    target_link_libraries(${LIB_NAME_CORE} PUBLIC ${Libav_LIBRARIES} ${LibArchive_LIBRARIES} )
endif()
