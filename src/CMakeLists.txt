

set(SOURCES

main.cpp

app/app_info.cpp

io/lottie/lottie_exporter.cpp
io/base.cpp

model/object.cpp
model/document.cpp
model/layers.cpp
model/animation.cpp
model/document_node.cpp
model/item_models/document_node_model.cpp
model/item_models/property_model.cpp

ui/dialogs/glaxnimate_window.cpp
ui/dialogs/glaxnimate_window.ui
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/app/application_info_generated.hpp
)

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} Qt5::Widgets QtColorWidgets)
