
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


TEMPLATE = lib

QT += widgets xml uitools concurrent
# requires(qtConfig(listview))

CONFIG += c++17

INCLUDEPATH += $$PWD/../core $$PWD/../gui $$PWD/../../external/QtAppSetup/src $$PWD/../../external/Qt-Color-Widgets/include

LIBS += -L../../external/Qt-Color-Widgets -L../core -L../../external/QtAppSetup -lglaxnimate_core -lQtAppSetup -lQtColorWidgets -lz

TARGET = glaxnimate_gui

!android {
    DEFINES += Q_OS_ANDROID
}

SOURCES = \
#glaxnimate_app.cpp \
#settings/clipboard_settings.cpp \
#settings/document_templates.cpp \
#settings/toolbar_settings.cpp \
#main.cpp \
item_models/document_model_base.cpp \
item_models/property_model_full.cpp \
item_models/property_model_base.cpp \
item_models/document_node_model.cpp \
item_models/property_model_single.cpp \
item_models/gradient_list_model.cpp \
    widgets/dialogs/document_environment.cpp \
widgets/shape_style/document_swatch_widget.cpp \
widgets/shape_style/gradient_list_widget.cpp \
widgets/shape_style/color_selector.cpp \
widgets/shape_style/fill_style_widget.cpp \
widgets/shape_style/stroke_style_widget.cpp \
widgets/composition_tab_bar.cpp \
#widgets/dialogs/gw_impl_debug.cpp \
#widgets/dialogs/document_metadata_dialog.cpp \
#widgets/dialogs/gw_impl_selection.cpp \
#widgets/dialogs/gw_impl_ui.cpp \
#widgets/dialogs/timing_dialog.cpp \
#widgets/dialogs/gw_impl_document.cpp \
#widgets/dialogs/trace_dialog.cpp \
#widgets/dialogs/color_quantization_dialog.cpp \
#widgets/dialogs/about_dialog.cpp \
#widgets/dialogs/resize_dialog.cpp \
#widgets/dialogs/clipboard_inspector.cpp \
#widgets/dialogs/io_status_dialog.cpp \
widgets/dialogs/shape_parent_dialog.cpp \
#widgets/dialogs/glaxnimate_window.cpp \
#widgets/dialogs/gw_impl_model.cpp \
#widgets/dialogs/plugin_ui_dialog.cpp \
#widgets/dialogs/startup_dialog.cpp \
#widgets/settings/toolbar_settings_widget.cpp \
#widgets/settings/plugin_settings_widget.cpp \
widgets/tab_bar_close_button.cpp \
widgets/flow_layout.cpp \
widgets/spin2d.cpp \
widgets/timeline/frame_controls_widget.cpp \
widgets/timeline/timeline_widget.cpp \
widgets/timeline/keyframe_editor_widget.cpp \
widgets/timeline/timeline_items.cpp \
widgets/timeline/compound_timeline_widget.cpp \
widgets/timeline/keyframe_transition_widget.cpp \
#widgets/script_console.cpp \
widgets/clickable_tab_bar.cpp \
widgets/font/font_model.cpp \
widgets/font/font_delegate.cpp \
widgets/font/font_style_dialog.cpp \
widgets/font/font_style_widget.cpp \
widgets/font/font_preview_widget.cpp \
#widgets/window_message_widget.cpp \
#widgets/node_menu.cpp \
#widgets/snippet_list_widget.cpp \
widgets/enum_combo.cpp \
widgets/view_transform_widget.cpp \
widgets/tools/text_tool_widget.cpp \
#widgets/tools/color_picker_widget.cpp \
widgets/tools/star_tool_widget.cpp \
widgets/tools/fill_tool_widget.cpp \
widgets/tools/shape_tool_widget.cpp \
widgets/canvas.cpp \
style/property_delegate.cpp \
style/dock_widget_style.cpp \
graphics/gradient_editor.cpp \
graphics/transform_graphics_item.cpp \
graphics/create_items.cpp \
graphics/document_node_graphics_item.cpp \
graphics/bezier_item.cpp \
graphics/handle.cpp \
graphics/document_scene.cpp \
tools/ellipse_tool.cpp \
tools/draw_tool.cpp \
tools/star_tool.cpp \
tools/edit_tool.cpp \
#tools/fill_tool.cpp \
#tools/color_picker.cpp \
tools/rectangle_tool.cpp \
tools/text_tool.cpp \
tools/base.cpp \
tools/freehand.cpp \
tools/select_tool.cpp

HEADERS = \
#settings/document_templates.hpp \
#settings/plugin_settings_group.hpp \
#settings/clipboard_settings.hpp \
#settings/toolbar_settings.hpp \
item_models/document_model_base.hpp \
item_models/property_model_base.hpp \
item_models/comp_filter_model.hpp \
item_models/asset_proxy_model.hpp \
item_models/drag_data.hpp \
item_models/property_model_single.hpp \
item_models/property_model_private.hpp \
item_models/gradient_list_model.hpp \
item_models/python_snippet_model.hpp \
item_models/document_node_model.hpp \
item_models/proxy_base.hpp \
item_models/property_model_full.hpp \
widgets/shape_style/shape_style_preview_widget.hpp \
widgets/shape_style/stroke_style_widget.hpp \
widgets/shape_style/document_swatch_widget.hpp \
widgets/shape_style/fill_style_widget.hpp \
widgets/shape_style/gradient_list_widget.hpp \
widgets/shape_style/color_selector.hpp \
widgets/canvas.hpp \
widgets/smaller_spinbox.hpp \
#widgets/dialogs/glaxnimate_window.hpp \
#widgets/dialogs/trace_dialog.hpp \
widgets/dialogs/keyframe_editor_dialog.hpp \
widgets/dialogs/document_environment.hpp \
#widgets/dialogs/document_metadata_dialog.hpp \
#widgets/dialogs/timing_dialog.hpp \
#widgets/dialogs/glaxnimate_window_p.hpp \
#widgets/dialogs/plugin_ui_dialog.hpp \
widgets/dialogs/shape_parent_dialog.hpp \
#widgets/dialogs/io_status_dialog.hpp \
#widgets/dialogs/about_dialog.hpp \
#widgets/dialogs/startup_dialog.hpp \
#widgets/dialogs/clipboard_inspector.hpp \
#widgets/dialogs/import_export_dialog.hpp \
#widgets/dialogs/resize_dialog.hpp \
#widgets/dialogs/color_quantization_dialog.hpp \
#widgets/settings/plugin_settings_widget.hpp \
#widgets/settings/toolbar_settings_widget.hpp \
#widgets/node_menu.hpp \
#widgets/snippet_list_widget.hpp \
widgets/timeline/frame_controls_widget.hpp \
widgets/timeline/keyframe_editor_widget.hpp \
widgets/timeline/compound_timeline_widget.hpp \
widgets/timeline/timeline_items.hpp \
widgets/timeline/keyframe_transition_widget.hpp \
widgets/timeline/timeline_widget.hpp \
widgets/scalable_button.hpp \
widgets/font/font_delegate.hpp \
widgets/font/font_style_widget.hpp \
widgets/font/font_preview_widget.hpp \
widgets/font/font_style_dialog.hpp \
widgets/font/font_model.hpp \
widgets/tab_bar_close_button.hpp \
widgets/custom_treeview.hpp \
widgets/enum_combo.hpp \
widgets/composition_tab_bar.hpp \
widgets/view_transform_widget.hpp \
widgets/spin2d.hpp \
#widgets/window_message_widget.hpp \
widgets/clickable_tab_bar.hpp \
#widgets/tools/color_picker_widget.hpp \
widgets/tools/fill_tool_widget.hpp \
widgets/tools/text_tool_widget.hpp \
widgets/tools/star_tool_widget.hpp \
widgets/tools/shape_tool_widget.hpp \
widgets/tools/shape_tool_widget_p.hpp \
#widgets/script_console.hpp \
#widgets/flow_layout.hpp \
style/dock_widget_style.hpp \
style/property_delegate.hpp \
style/better_elide_delegate.hpp \
style/fixed_height_delegate.hpp \
graphics/typed_item.hpp \
graphics/sizepos_item.hpp \
graphics/text_attributes_editor.hpp \
graphics/bezier_item.hpp \
graphics/main_composition_item.hpp \
graphics/rect_rounder.hpp \
graphics/create_items.hpp \
graphics/document_scene.hpp \
graphics/transform_graphics_item.hpp \
graphics/shape_graphics_item.hpp \
graphics/position_item.hpp \
graphics/item_data.hpp \
graphics/star_radius_item.hpp \
graphics/graphics_editor.hpp \
graphics/gradient_editor.hpp \
graphics/composition_item.hpp \
graphics/handle.hpp \
graphics/document_node_graphics_item.hpp \
#glaxnimate_app.hpp \
tools/handle_menu.hpp \
tools/edit_tool.hpp \
tools/draw_tool.hpp \
tools/rectangle_tool.hpp \
tools/base.hpp \
tools/draw_tool_drag.hpp \
tools/draw_tool_base.hpp

FORMS = \
widgets/shape_style/document_swatch_widget.ui \
widgets/shape_style/color_selector.ui \
widgets/shape_style/stroke_style_widget.ui \
widgets/shape_style/gradient_list_widget.ui \
#widgets/snippet_list_widget.ui \
#widgets/dialogs/document_metadata_dialog.ui \
#widgets/dialogs/color_quantization_dialog.ui \
#widgets/dialogs/timing_dialog.ui \
#widgets/dialogs/io_status_dialog.ui \
#widgets/dialogs/resize_dialog.ui \
#widgets/dialogs/about_dialog.ui \
#widgets/dialogs/startup_dialog.ui \
#widgets/dialogs/glaxnimate_window.ui \
#widgets/dialogs/trace_dialog.ui \
widgets/dialogs/shape_parent_dialog.ui \
#widgets/settings/plugin_settings_widget.ui \
#widgets/settings/toolbar_settings_widget.ui \
widgets/timeline/keyframe_editor_widget.ui \
widgets/timeline/frame_controls_widget.ui \
widgets/timeline/compound_timeline_widget.ui \
widgets/font/font_style_widget.ui \
widgets/font/font_style_dialog.ui \
widgets/font/font_preview_widget.ui \
#widgets/script_console.ui \
widgets/view_transform_widget.ui \
#widgets/tools/color_picker_widget.ui \
widgets/tools/fill_tool_widget.ui \
widgets/tools/shape_tool_widget.ui \
#widgets/window_message_widget.ui

#RESOURCES = \
#build/bin/gui/resources/glaxnimate.qrc
