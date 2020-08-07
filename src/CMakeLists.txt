

set(SOURCES

main.cpp
app_info.cpp
glaxnimate_app.cpp

io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/base.cpp

model/animation.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/layers.cpp
model/object.cpp
model/transform.cpp
model/item_models/document_node_model.cpp
model/item_models/property_model.cpp
model/graphics/handle.cpp
model/graphics/document_node_graphics_item.cpp
model/graphics/document_scene.cpp
model/graphics/transform_graphics_item.cpp
model/animation/keyframe_transition.cpp
model/animation/animatable.cpp

tools/base.cpp
tools/dummy.cpp

ui/dialogs/glaxnimate_window.cpp
ui/dialogs/glaxnimate_window.ui
ui/dialogs/io_status_dialog.cpp
ui/dialogs/io_status_dialog.ui
ui/dialogs/about_dialog.cpp
ui/dialogs/about_dialog.ui
ui/widgets/glaxnimate_graphics_view.cpp
ui/widgets/view_transform_widget.cpp
ui/widgets/view_transform_widget.ui
ui/widgets/keyframe_transition_widget.cpp
ui/widgets/keyframe_editor_widget.cpp
ui/widgets/keyframe_editor_widget.ui
ui/widgets/frame_controls_widget.cpp
ui/widgets/frame_controls_widget.ui
ui/widgets/flow_layout.cpp
ui/dialogs/gw_impl_color.cpp
ui/dialogs/gw_impl_document.cpp
ui/dialogs/gw_impl_model.cpp
ui/dialogs/gw_impl_script.cpp
ui/dialogs/gw_impl_ui.cpp
)

if(${PYTHON_ENABLED})
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
    if ( ${WTF_IS_THIS_WARNING} )
        SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
    endif()

    list(APPEND SOURCES python.cpp)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt5::Widgets Qt5::Svg QtColorWidgets QtHistoryLineEdit QtAppSetup)
