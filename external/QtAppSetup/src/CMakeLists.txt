set(SOURCES

app/application.cpp
app/settings/settings.cpp
app/settings/palette_settings.cpp
app/settings/keyboard_shortcuts.cpp
app/settings/keyboard_shortcuts_model.cpp
app/translation_service.cpp
app/scripting/script_engine.cpp
app/widgets/settings_dialog.cpp
app/widgets/settings_dialog.ui
app/widgets/widget_palette_editor.cpp
app/widgets/widget_palette_editor.ui
app/widgets/clearable_keysequence_edit.cpp
app/widgets/clearable_keysequence_edit.ui
app/widgets/keyboard_settings_widget.cpp
app/widgets/keyboard_settings_widget.ui
app/log/logger.cpp
app/log/log_model.cpp
app/cli.cpp
)

add_library(${PROJECT_SLUG} OBJECT ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt::Widgets QtColorWidgets)
target_include_directories(${PROJECT_SLUG} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
