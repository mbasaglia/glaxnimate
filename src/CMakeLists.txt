

set(SOURCES

main.cpp

io/lottie/lottie_exporter.cpp
io/base.cpp

model/object.cpp
model/document.cpp
model/layers.cpp
model/animation.cpp
model/document_node.cpp
model/item_models/document_node_model.cpp

ui/dialogs/glaxnimate_window.cpp
ui/dialogs/glaxnimate_window.ui
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.h
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.h
)

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} Qt5::Widgets QtColorWidgets)
