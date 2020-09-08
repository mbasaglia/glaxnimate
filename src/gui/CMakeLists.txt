set(SOURCES

main.cpp
glaxnimate_app.cpp

graphics/handle.cpp
graphics/document_node_graphics_item.cpp
graphics/document_scene.cpp
graphics/transform_graphics_item.cpp
graphics/create_items.cpp
graphics/bezier_item.cpp

item_models/document_node_model.cpp
item_models/property_model.cpp

style/property_delegate.cpp

tools/base.cpp
tools/dummy.cpp
tools/select.cpp
tools/ellipse.cpp
tools/rectangle.cpp
tools/draw_tool.cpp

widgets/dialogs/glaxnimate_window.cpp
widgets/dialogs/glaxnimate_window.ui
widgets/dialogs/io_status_dialog.cpp
widgets/dialogs/io_status_dialog.ui
widgets/dialogs/about_dialog.cpp
widgets/dialogs/about_dialog.ui
widgets/dialogs/gw_impl_document.cpp
widgets/dialogs/gw_impl_model.cpp
widgets/dialogs/gw_impl_script.cpp
widgets/dialogs/gw_impl_ui.cpp

widgets/timeline/keyframe_transition_widget.cpp
widgets/timeline/keyframe_editor_widget.cpp
widgets/timeline/keyframe_editor_widget.ui
widgets/timeline/frame_controls_widget.cpp
widgets/timeline/frame_controls_widget.ui
widgets/timeline/timeline_widget.cpp
widgets/timeline/compound_timeline_widget.cpp
widgets/timeline/compound_timeline_widget.ui

widgets/glaxnimate_graphics_view.cpp
widgets/view_transform_widget.cpp
widgets/view_transform_widget.ui
widgets/flow_layout.cpp
widgets/spin2d.cpp
widgets/tools/shape_tool_widget.cpp
widgets/tools/shape_tool_widget.ui
widgets/enum_combo.cpp
widgets/color_selector.cpp
widgets/color_selector.ui
widgets/stroke_style_widget.cpp
widgets/stroke_style_widget.ui
)

if(${PYTHON_ENABLED})
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
    if ( ${WTF_IS_THIS_WARNING} )
        SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
    endif()

    list(APPEND SOURCES gui_python.cpp)
endif()

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
add_library(${LIB_NAME_GUI} OBJECT ${SOURCES})
target_link_libraries(${LIB_NAME_GUI} PUBLIC Qt5::Widgets QtColorWidgets QtHistoryLineEdit ${LIB_NAME})
