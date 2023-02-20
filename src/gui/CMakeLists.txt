set(SOURCES

glaxnimate_app.cpp

emoji/emoji_data.cpp
emoji/emoji_dialog.cpp
emoji/emoji_set.cpp

graphics/handle.cpp
graphics/document_node_graphics_item.cpp
graphics/document_scene.cpp
graphics/transform_graphics_item.cpp
graphics/create_items.cpp
graphics/bezier_item.cpp
graphics/gradient_editor.cpp

item_models/document_node_model.cpp
item_models/property_model_base.cpp
item_models/property_model_single.cpp
item_models/property_model_full.cpp
item_models/gradient_list_model.cpp
item_models/document_model_base.cpp


settings/clipboard_settings.cpp
settings/toolbar_settings.cpp
settings/document_templates.cpp
settings/api_credentials.cpp

style/property_delegate.cpp
style/dock_widget_style.cpp
style/scroll_area_event_filter.cpp

tools/base.cpp
tools/freehand.cpp
tools/select_tool.cpp
tools/ellipse_tool.cpp
tools/rectangle_tool.cpp
tools/draw_tool.cpp
tools/edit_tool.cpp
tools/star_tool.cpp
tools/text_tool.cpp
tools/draw_tool_base.cpp

widgets/dialogs/io_status_dialog.cpp
widgets/dialogs/io_status_dialog.ui
widgets/dialogs/selection_manager.cpp
widgets/dialogs/shape_parent_dialog.cpp
widgets/dialogs/shape_parent_dialog.ui
widgets/dialogs/resize_dialog.cpp
widgets/dialogs/resize_dialog.ui
widgets/dialogs/timing_dialog.cpp
widgets/dialogs/timing_dialog.ui
widgets/dialogs/plugin_ui_dialog.cpp
widgets/dialogs/document_metadata_dialog.cpp
widgets/dialogs/document_metadata_dialog.ui
widgets/dialogs/trace_dialog.cpp
widgets/dialogs/trace_dialog.ui
widgets/dialogs/clipboard_inspector.cpp
widgets/dialogs/color_quantization_dialog.cpp
widgets/dialogs/color_quantization_dialog.ui
widgets/dialogs/startup_dialog.cpp
widgets/dialogs/startup_dialog.ui
widgets/dialogs/export_image_sequence_dialog.cpp
widgets/dialogs/export_image_sequence_dialog.ui
widgets/dialogs/select_shape_dialog.cpp
widgets/dialogs/select_shape_dialog.ui
widgets/dialogs/follow_path_dialog.cpp
widgets/dialogs/follow_path_dialog.ui

widgets/lottiefiles/lottiefiles_search_dialog.cpp
widgets/lottiefiles/lottiefiles_search_dialog.ui
widgets/lottiefiles/search_result.cpp
widgets/lottiefiles/graphql.cpp

widgets/timeline/keyframe_transition_widget.cpp
widgets/timeline/keyframe_editor_widget.cpp
widgets/timeline/keyframe_editor_widget.ui
widgets/timeline/frame_controls_widget.cpp
widgets/timeline/frame_controls_widget.ui
widgets/timeline/timeline_widget.cpp
widgets/timeline/compound_timeline_widget.cpp
widgets/timeline/compound_timeline_widget.ui
widgets/timeline/timeline_items.cpp
widgets/timeline/timeline_treeview.cpp

widgets/tools/shape_tool_widget.cpp
widgets/tools/shape_tool_widget.ui
widgets/tools/star_tool_widget.cpp
widgets/tools/color_picker_widget.cpp
widgets/tools/color_picker_widget.ui
widgets/tools/fill_tool_widget.cpp
widgets/tools/fill_tool_widget.ui
widgets/tools/text_tool_widget.cpp

widgets/canvas.cpp
widgets/view_transform_widget.cpp
widgets/view_transform_widget.ui
widgets/flow_layout.cpp
widgets/spin2d.cpp
widgets/enum_combo.cpp
widgets/window_message_widget.cpp
widgets/window_message_widget.ui
widgets/tab_bar/tab_bar_close_button.cpp
widgets/tab_bar/clickable_tab_bar.cpp
widgets/tab_bar/composition_tab_bar.cpp
widgets/docks/layer_view.cpp

widgets/settings/plugin_settings_widget.cpp
widgets/settings/plugin_settings_widget.ui
widgets/settings/toolbar_settings_widget.cpp
widgets/settings/toolbar_settings_widget.ui

widgets/shape_style/color_selector.cpp
widgets/shape_style/color_selector.ui
widgets/shape_style/stroke_style_widget.cpp
widgets/shape_style/stroke_style_widget.ui
widgets/shape_style/fill_style_widget.cpp
widgets/shape_style/document_swatch_widget.cpp
widgets/shape_style/document_swatch_widget.ui
widgets/shape_style/gradient_list_widget.cpp
widgets/shape_style/gradient_list_widget.ui

widgets/font/font_model.cpp
widgets/font/font_delegate.cpp
widgets/font/font_preview_widget.cpp
widgets/font/font_preview_widget.ui
widgets/font/font_style_dialog.cpp
widgets/font/font_style_dialog.ui
widgets/font/font_style_widget.cpp
widgets/font/font_style_widget.ui
widgets/font/google_fonts_model.cpp
widgets/font/google_fonts_widget.cpp
widgets/font/google_fonts_widget.ui
widgets/font/font_size_widget.cpp
widgets/font/font_size_widget.ui
widgets/font/external_font_widget.cpp
widgets/font/external_font_widget.ui
widgets/font/font_loader.cpp
)

if ( NOT ANDROID )
    list(APPEND SOURCES
        main.cpp
        cli.cpp
        gui_python.cpp

        emoji/emoji_set_dialog.cpp
        emoji/emoji_set_dialog.ui

        tools/color_picker.cpp
        tools/fill_tool.cpp

        widgets/dialogs/about_dialog.cpp
        widgets/dialogs/about_dialog.ui
        widgets/dialogs/glaxnimate_window.cpp
        widgets/dialogs/glaxnimate_window.ui
        widgets/dialogs/gw_impl_document.cpp
        widgets/dialogs/gw_impl_model.cpp
        widgets/dialogs/gw_impl_ui.cpp
        widgets/dialogs/gw_impl_debug.cpp
        widgets/dialogs/gw_impl_selection.cpp

        widgets/docks/script_console.cpp
        widgets/docks/script_console.ui
        widgets/docks/snippet_list_widget.cpp
        widgets/docks/snippet_list_widget.ui
        widgets/menus/node_menu.cpp
    )
endif()

configure_file(resources/glaxnimate.in.qrc resources/glaxnimate.qrc)

if(COMMAND qt_add_resources)
    qt_add_resources(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources/glaxnimate.qrc)
else()
    qt5_add_resources(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources/glaxnimate.qrc)
endif()

set(API_KEY_GOOGLE_FONTS "$ENV{API_KEY_GOOGLE_FONTS}" CACHE STRING "API key for Google Fonts")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/settings/api_keys.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/api_keys.hpp
)



include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
add_library(${LIB_NAME_GUI} OBJECT ${SOURCES})
target_link_libraries(
    ${LIB_NAME_GUI} PUBLIC
    QtColorWidgets
    ${LIB_NAME_CORE}
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::UiTools
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
)



if ( NOT ANDROID )
    target_link_libraries(${LIB_NAME_GUI} PUBLIC QtHistoryLineEdit )
endif()
