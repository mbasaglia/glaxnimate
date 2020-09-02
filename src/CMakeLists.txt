set(LIB_SOURCES

app_info.cpp

io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/base.cpp

model/main_composition.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/layers/layer.cpp
model/layers/empty_layer.cpp
model/layers/shape_layer.cpp
model/layers/solid_color_layer.cpp
model/object.cpp
model/transform.cpp
model/shapes/shape.cpp
model/factory.cpp
model/item_models/document_node_model.cpp
model/item_models/property_model.cpp
model/graphics/handle.cpp
model/graphics/document_node_graphics_item.cpp
model/graphics/document_scene.cpp
model/graphics/transform_graphics_item.cpp
model/animation/keyframe_transition.cpp
model/animation/animatable.cpp
model/shapes/fill.cpp
model/shapes/rect.cpp

)

set(APP_SOURCES

main.cpp
glaxnimate_app.cpp

tools/base.cpp
tools/dummy.cpp
tools/select.cpp
tools/ellipse.cpp
tools/rectangle.cpp

ui/dialogs/glaxnimate_window.cpp
ui/dialogs/glaxnimate_window.ui
ui/dialogs/io_status_dialog.cpp
ui/dialogs/io_status_dialog.ui
ui/dialogs/about_dialog.cpp
ui/dialogs/about_dialog.ui
ui/dialogs/gw_impl_color.cpp
ui/dialogs/gw_impl_document.cpp
ui/dialogs/gw_impl_model.cpp
ui/dialogs/gw_impl_script.cpp
ui/dialogs/gw_impl_ui.cpp
ui/widgets/timeline/keyframe_transition_widget.cpp
ui/widgets/timeline/keyframe_editor_widget.cpp
ui/widgets/timeline/keyframe_editor_widget.ui
ui/widgets/timeline/frame_controls_widget.cpp
ui/widgets/timeline/frame_controls_widget.ui
ui/widgets/timeline/timeline_widget.cpp
ui/widgets/timeline/compound_timeline_widget.cpp
ui/widgets/timeline/compound_timeline_widget.ui
ui/widgets/glaxnimate_graphics_view.cpp
ui/widgets/view_transform_widget.cpp
ui/widgets/view_transform_widget.ui
ui/widgets/flow_layout.cpp
ui/widgets/spin2d.cpp
ui/widgets/tools/shape_tool_widget.cpp
ui/widgets/tools/shape_tool_widget.ui
)

if(${PYTHON_ENABLED})
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
    if ( ${WTF_IS_THIS_WARNING} )
        SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
    endif()

    list(APPEND LIB_SOURCES python.cpp)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)

set(LIB_NAME "${PROJECT_SLUG}lib")
add_library(${LIB_NAME} STATIC ${LIB_SOURCES})
target_link_libraries(${LIB_NAME} PUBLIC Qt5::Svg QtAppSetup)
set_property(TARGET ${LIB_NAME} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${APP_SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt5::Widgets QtColorWidgets QtHistoryLineEdit ${LIB_NAME})

