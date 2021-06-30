
OBJECTS_DIR = out/obj
MOC_DIR = out/generated
UI_DIR = out/generated
RCC_DIR = out/generated

android {
    OBJECTS_DIR = $$ANDROID_TARGET_ARCH/out/obj
    MOC_DIR = $$ANDROID_TARGET_ARCH/out/generated
    UI_DIR = $$ANDROID_TARGET_ARCH/out/generated
    RCC_DIR = $$ANDROID_TARGET_ARCH/out/generated
}


QT += widgets xml uitools concurrent
# requires(qtConfig(listview))

CONFIG += c++17

INCLUDEPATH += $$PWD/src $$PWD/../../build/bin/core $$PWD/../../external/QtAppSetup/src $$PWD/../../external/Qt-Color-Widgets/include


LIBS += -lz -L../../external/QtAppSetup -lQtAppSetup -L../../external/Qt-Color-Widgets -lQtColorWidgets

TEMPLATE=lib

TARGET=glaxnimate_core

OBJECTS_DIR = out/obj
MOC_DIR = out/generated
UI_DIR = out/generated
RCC_DIR = out/generated


SOURCES = \
app_info.cpp \
command/structure_commands.cpp \
command/shape_commands.cpp \
command/animation_commands.cpp \
io/lottie/lottie_format.cpp \
io/lottie/lottie_html_format.cpp \
io/lottie/tgs_format.cpp \
io/lottie/cbor_write_json.cpp \
io/glaxnimate/glaxnimate_format.cpp \
io/glaxnimate/glaxnimate_importer.cpp \
io/glaxnimate/glaxnimate_mime.cpp \
io/svg/svg_renderer.cpp \
io/svg/svg_parser.cpp \
io/svg/detail.cpp \
io/svg/svg_format.cpp \
io/mime/mime_serializer.cpp \
io/raster/raster_format.cpp \
io/base.cpp \
math/geom.cpp \
math/ellipse_solver.cpp \
math/bezier/bezier.cpp \
math/bezier/point.cpp \
math/bezier/operations.cpp \
math/bezier/cubic_struts.cpp \
math/bezier/meta.cpp \
model/main_composition.cpp \
model/composition.cpp \
model/document.cpp \
model/document_node.cpp \
model/object.cpp \
model/transform.cpp \
model/factory.cpp \
model/animation_container.cpp \
model/stretchable_time.cpp \
model/comp_graph.cpp \
model/mask_settings.cpp \
model/visitor.cpp \
model/animation/keyframe_transition.cpp \
model/animation/animatable.cpp \
model/animation/animatable_path.cpp \
model/property/property.cpp \
model/property/reference_property.cpp \
model/property/option_list_property.cpp \
model/assets/assets.cpp \
model/assets/brush_style.cpp \
model/assets/named_color.cpp \
model/assets/bitmap.cpp \
model/assets/gradient.cpp \
model/assets/asset_base.cpp \
model/assets/asset.cpp \
model/assets/precomposition.cpp \
model/shapes/shape.cpp \
model/shapes/fill.cpp \
model/shapes/rect.cpp \
model/shapes/group.cpp \
model/shapes/ellipse.cpp \
model/shapes/path.cpp \
model/shapes/stroke.cpp \
model/shapes/polystar.cpp \
model/shapes/styler.cpp \
model/shapes/layer.cpp \
model/shapes/image.cpp \
model/shapes/precomp_layer.cpp \
model/shapes/text.cpp \
model/shapes/repeater.cpp \
model/shapes/trim.cpp \
utils/gzip.cpp \
#utils/trace.cpp \
utils/quantize.cpp \
plugin/plugin.cpp \
plugin/action.cpp \
plugin/io.cpp

HEADERS = \
plugin/action.hpp \
plugin/executor.hpp \
plugin/snippet.hpp \
plugin/service.hpp \
plugin/plugin.hpp \
plugin/io.hpp \
utils/color.hpp \
utils/gzip.hpp \
utils/sort_gradient.hpp \
utils/pseudo_mutex.hpp \
utils/regexp.hpp \
#utils/trace.hpp \
utils/range.hpp \
utils/qstring_hash.hpp \
command/structure_commands.hpp \
command/shape_commands.hpp \
command/base.hpp \
command/property_commands.hpp \
command/undo_macro_guard.hpp \
command/object_list_commands.hpp \
command/animation_commands.hpp \
io/lottie/tgs_format.hpp \
io/lottie/lottie_private_common.hpp \
io/lottie/cbor_write_json.hpp \
io/lottie/lottie_format.hpp \
io/lottie/lottie_exporter.hpp \
io/lottie/lottie_html_format.hpp \
io/lottie/lottie_importer.hpp \
io/svg/svg_format.hpp \
io/svg/svg_html_format.hpp \
io/svg/svg_mime.hpp \
io/svg/svg_parser.hpp \
io/svg/font_weight.hpp \
io/svg/css_parser.hpp \
io/svg/animate_parser.hpp \
io/svg/detail.hpp \
io/svg/svg_renderer.hpp \
io/svg/path_parser.hpp \
io/mime/mime_serializer.hpp \
io/mime/json_mime.hpp \
io/io_registry.hpp \
io/base.hpp \
io/glaxnimate/glaxnimate_format.hpp \
io/glaxnimate/glaxnimate_mime.hpp \
io/glaxnimate/import_state.hpp \
io/raster/raster_format.hpp \
io/raster/raster_mime.hpp \
io/options.hpp \
application_info_generated.in.hpp \
model/object.hpp \
model/mask_settings.hpp \
model/property/sub_object_property.hpp \
model/property/reference_property.hpp \
model/property/object_list_property.hpp \
model/property/option_list_property.hpp \
model/property/property.hpp \
model/assets/bitmap.hpp \
model/assets/precomposition.hpp \
model/assets/assets.hpp \
model/assets/asset.hpp \
model/assets/asset_base.hpp \
model/assets/brush_style.hpp \
model/assets/gradient.hpp \
model/assets/named_color.hpp \
model/transform.hpp \
model/visitor.hpp \
model/document_node.hpp \
model/shapes/rect.hpp \
model/shapes/trim.hpp \
model/shapes/shape.hpp \
model/shapes/ellipse.hpp \
model/shapes/styler.hpp \
model/shapes/precomp_layer.hpp \
model/shapes/group.hpp \
model/shapes/polystar.hpp \
model/shapes/repeater.hpp \
model/shapes/image.hpp \
model/shapes/text.hpp \
model/shapes/path.hpp \
model/shapes/layer.hpp \
model/shapes/stroke.hpp \
model/shapes/fill.hpp \
model/animation_container.hpp \
model/document.hpp \
model/stretchable_time.hpp \
model/main_composition.hpp \
model/animation/frame_time.hpp \
model/animation/keyframe_transition.hpp \
model/animation/animatable.hpp \
model/animation/join_animatables.hpp \
model/animation/animatable_path.hpp \
model/comp_graph.hpp \
model/simple_visitor.hpp \
model/composition.hpp \
model/invoke.hpp \
model/factory.hpp \
app_info.hpp \
math/geom.hpp \
math/math.hpp \
math/ellipse_solver.hpp \
math/vector.hpp \
math/bezier/point.hpp \
math/bezier/solver.hpp \
math/bezier/bezier.hpp \
math/bezier/cubic_struts.hpp \
math/bezier/segment.hpp \
math/bezier/operations.hpp \
math/bezier/meta.hpp
