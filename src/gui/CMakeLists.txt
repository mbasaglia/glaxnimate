set(SOURCES

main.cpp
glaxnimate_app.cpp

graphics/handle.cpp
graphics/document_node_graphics_item.cpp
graphics/document_scene.cpp
graphics/transform_graphics_item.cpp
graphics/create_items.cpp

item_models/document_node_model.cpp
item_models/property_model.cpp

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

    list(APPEND SOURCES gui_python.cpp)
endif()

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
add_library(${LIB_NAME_GUI} OBJECT ${SOURCES})
target_link_libraries(${LIB_NAME_GUI} PUBLIC Qt5::Widgets QtColorWidgets QtHistoryLineEdit ${LIB_NAME})
