

set(SOURCES
main.cpp
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.h
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.h
)

#add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)
add_executable(${PROJECT_SLUG} ${SOURCES})
target_link_libraries(${PROJECT_SLUG} Qt5::Widgets QtColorWidgets)
