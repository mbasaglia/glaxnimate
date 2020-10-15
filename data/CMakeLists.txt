
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons/icons-dark DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)

install(DIRECTORY icons/breeze-icons/icons DESTINATION ${DATA_INSTALL}/icons PATTERN *@* EXCLUDE)
install(DIRECTORY icons/breeze-icons/icons-dark DESTINATION ${DATA_INSTALL}/icons PATTERN *@* EXCLUDE)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/plugins DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY plugins DESTINATION ${DATA_INSTALL})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY images DESTINATION ${DATA_INSTALL})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/themes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY themes DESTINATION ${DATA_INSTALL})

# Translation
include(../cmake/qt.cmake)
set(ALL_SOURCE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src)
create_qt_linguist_translations(
    translations
    translations/glaxnimate_en.ts
    translations/glaxnimate_it.ts
    translations/glaxnimate_de.ts
    translations/glaxnimate_id.ts
    translations/glaxnimate_ru.ts
)
install(FILES ${LINGUIST_OUTPUT_FILES} DESTINATION ${DATA_INSTALL}/translations/)

# Desktop
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/glaxnimate.in.desktop ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop DESTINATION ${DATA_INSTALL})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop DESTINATION share/applications)

# Icon
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/images/logo.svg ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg COPYONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg DESTINATION share/icons/hicolor/scalable/ )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
    COMMAND inkscape ${CMAKE_CURRENT_SOURCE_DIR}/images/logo.svg -o ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/logo.svg
)
add_custom_target(
    logo_png
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png DESTINATION share/icons/hicolor/512x512/ )
