set(SOURCES)
get_property(SOURCES_tmp TARGET ${PROJECT_SLUG} PROPERTY SOURCES)
foreach(src ${SOURCES_tmp})
    get_filename_component(src ${src} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR}/src/)
    list(APPEND SOURCES ${src})
endforeach()


add_custom_target(
    icons
    #ALL
    DEPENDS ${SOURCES}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/icons/make_icons mktheme ${CMAKE_SOURCE_DIR}/src -o ${CMAKE_CURRENT_BINARY_DIR}/img/icons/breeze
)


# Translation
include(../cmake/qt.cmake)
set(ALL_SOURCE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src)
create_qt_linguist_translations(
    translations
)
