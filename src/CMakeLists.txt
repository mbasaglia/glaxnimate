

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
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} Qt5::Widgets QtColorWidgets)
