

set(SOURCES

main.cpp
app_info.cpp
glaxnimate_app.cpp

app/application.cpp
app/settings/settings.cpp
app/translation_service.cpp

io/lottie/lottie_format.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/base.cpp

model/animation.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/layers.cpp
model/object.cpp
model/item_models/document_node_model.cpp
model/item_models/property_model.cpp
model/graphics/handle.cpp
model/graphics/document_node_graphics_item.cpp
model/graphics/document_scene.cpp
model/animation/keyframe_transition.cpp

ui/dialogs/glaxnimate_window.cpp
ui/dialogs/glaxnimate_window.ui
ui/dialogs/settings_dialog.cpp
ui/dialogs/settings_dialog.ui
ui/dialogs/io_status_dialog.cpp
ui/dialogs/io_status_dialog.ui
ui/dialogs/about_dialog.cpp
ui/dialogs/about_dialog.ui
ui/widgets/glaxnimate_graphics_view.cpp
ui/widgets/view_transform_widget.cpp
ui/widgets/view_transform_widget.ui

scripting/plugin.cpp
scripting/widget/plugin_settings_widget.cpp
scripting/widget/plugin_settings_widget.ui
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
if ( ${WTF_IS_THIS_WARNING} )
    SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
endif()

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt5::Widgets QtColorWidgets QtHistoryLineEdit)
