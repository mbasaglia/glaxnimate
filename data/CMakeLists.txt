# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

if ( ANDROID )

    # The breeze-icons package approach is inspired by https://invent.kde.org/frameworks/kirigami/-/blob/kf5/KF5Kirigami2Macros.cmake
    include(CMakeParseArguments)
    include(ExternalProject)


    function(android_package_breeze_icons)
        set(_multiValueArgs ICONS)
        cmake_parse_arguments(ARG "" "" "${_multiValueArgs}" ${ARGN} )

        if(NOT ARG_ICONS)
            message(FATAL_ERROR "No ICONS argument given to android_package_breeze_icons")
        endif()

        function(_find_breeze_icon icon varName)
            #HACKY
            SET(path "")
            file(GLOB_RECURSE path ${_BREEZEICONS_DIR}/icons/*/48/${icon}.svg )

            #search in other sizes as well
            if (path STREQUAL "")
                file(GLOB_RECURSE path ${_BREEZEICONS_DIR}/icons/*/32/${icon}.svg )
                if (path STREQUAL "")
                    file(GLOB_RECURSE path ${_BREEZEICONS_DIR}/icons/*/22/${icon}.svg )
                    if (path STREQUAL "")
                        file(GLOB_RECURSE path ${_BREEZEICONS_DIR}/icons/*/16/${icon}.svg )
                    endif()
                endif()
            endif()
            if (path STREQUAL "")
                file(GLOB_RECURSE path ${_BREEZEICONS_DIR}/icons/*/symbolic/${icon}.svg )
            endif()
            if (path STREQUAL "")
                return()
            endif()

            list(LENGTH path _count_paths)
            if (_count_paths GREATER 1)
                message(WARNING "Found more than one version of '${icon}': ${path}")
            endif()
            list(GET path 0 path)
            get_filename_component(path "${path}" REALPATH)

            SET(${varName} ${path} PARENT_SCOPE)
        endfunction()

        if (BREEZEICONS_DIR AND NOT EXISTS ${BREEZEICONS_DIR})
            message(FATAL_ERROR "BREEZEICONS_DIR variable does not point to existing dir: \"${BREEZEICONS_DIR}\"")
        endif()

        set(_BREEZEICONS_DIR "${BREEZEICONS_DIR}")

        if(NOT _BREEZEICONS_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons/icons")
            set(_BREEZEICONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons")
        endif()

        #FIXME: this is a terrible hack
        if(NOT _BREEZEICONS_DIR)
            set(_BREEZEICONS_DIR "${CMAKE_BINARY_DIR}/breeze-icons/src/breeze-icons")

            # replacement for ExternalProject_Add not yet working
            # first time config?
            if (NOT EXISTS ${_BREEZEICONS_DIR})
                find_package(Git)
                execute_process(COMMAND ${GIT_EXECUTABLE} clone --depth 1 --branch kf5 https://invent.kde.org/frameworks/breeze-icons.git ${_BREEZEICONS_DIR})
            endif()

            # external projects are only pulled at make time, not configure time
            # so this is too late to work with the _find_breeze_icon() method
            # _find_breeze_icon() would need to be turned into a target/command
            if (FALSE)
            ExternalProject_Add(
                breeze-icons
                PREFIX breeze-icons
                GIT_REPOSITORY https://invent.kde.org/frameworks/breeze-icons.git
                CONFIGURE_COMMAND ""
                BUILD_COMMAND ""
                INSTALL_COMMAND ""
                LOG_DOWNLOAD ON
            )
            endif()
        endif()

        message (STATUS "Using ${_BREEZEICONS_DIR} as breeze dir")
        message (STATUS "Found external breeze icons:")
        foreach(_iconName ${ARG_ICONS})
            set(_iconPath "")
            _find_breeze_icon(${_iconName} _iconPath)
            message (STATUS ${_iconPath})
            if (EXISTS ${_iconPath})
                install(FILES ${_iconPath} DESTINATION ${DATA_INSTALL}/icons/icons RENAME ${_iconName}.svg)
            else()
                message (WARNING "${_iconName} not found")
            endif()
        endforeach()

        #generate an index.theme that qiconloader can understand
        file(WRITE ${CMAKE_BINARY_DIR}/index.theme "[Icon Theme]\nName=Breeze\nDirectories=icons\n[icons]\nSize=32\nType=Scalable")
        install(FILES ${CMAKE_BINARY_DIR}/index.theme DESTINATION ${DATA_INSTALL}/icons)
    endfunction()

    android_package_breeze_icons(ICONS
        telegram
        edit-cut
        edit-copy
        edit-paste
        edit-delete
        edit-undo
        edit-redo
        edit-select
        edit-node
        edit-clear-all
        document-new
        document-open
        document-save
        document-save-as
        document-import
        document-export
        document-properties
        keyframe-add
        keyframe-remove
        keyframe-record
        view-preview
        player-time
        fill-color
        object-stroke-style
        question
        draw-brush
        draw-bezier-curves
        draw-freehand
        draw-rectangle
        draw-ellipse
        draw-polygon-star
        draw-text
        shapes
        overflow-menu
        media-playback-start
        media-playlist-repeat
        go-first
        go-previous
        go-next
        go-last
        layer-lower
        layer-raise
        dialog-layers
        smiley-shape
    )

    #get_target_property(ASSETS_PATH ${PROJECT_SLUG}_apk assets)
    #file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/images DESTINATION ${ASSETS_PATH})
    #file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/templates DESTINATION ${ASSETS_PATH})
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/icons/breeze-icons/icons-dark DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/)
    install(DIRECTORY icons/breeze-icons/icons DESTINATION ${DATA_INSTALL}/icons PATTERN *@* EXCLUDE)
    install(DIRECTORY icons/breeze-icons/icons-dark DESTINATION ${DATA_INSTALL}/icons PATTERN *@* EXCLUDE)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/themes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    install(DIRECTORY themes DESTINATION ${DATA_INSTALL})

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/python-lottie/lib/lottie DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib/)
    install(DIRECTORY lib/python-lottie/lib/lottie DESTINATION ${DATA_INSTALL}/lib/)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/emoji DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    install(DIRECTORY emoji DESTINATION ${DATA_INSTALL})
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY images DESTINATION ${DATA_INSTALL})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY templates DESTINATION ${DATA_INSTALL})

# Translation
include(../cmake/qt.cmake)
set(ALL_SOURCE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src)
create_qt_linguist_translations(
    TARGET translations
    TRANSLATIONS
    translations/glaxnimate_en_US.ts
    translations/glaxnimate_en_GB.ts
    translations/glaxnimate_es.ts
    translations/glaxnimate_it.ts
    translations/glaxnimate_de.ts
    translations/glaxnimate_fr.ts
    translations/glaxnimate_zh_CN.ts
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/translations/
    SOURCES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
)
install(FILES ${LINGUIST_OUTPUT_FILES} DESTINATION ${DATA_INSTALL}/translations/)

# Desktop
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/glaxnimate.in.desktop ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop DESTINATION ${DATA_INSTALL})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.desktop DESTINATION share/applications RENAME ${PROJECT_ID}.desktop)

# Misc
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../AUTHORS.md ${CMAKE_CURRENT_BINARY_DIR}/AUTHORS.md COPYONLY)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../AUTHORS.md DESTINATION ${DATA_INSTALL})

# Icon
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/logo/logo.svg ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/logo/file-icon.svg ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}-file.svg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/logo/logo.svg ${CMAKE_CURRENT_SOURCE_DIR}/images/logo.svg COPYONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg DESTINATION share/icons/hicolor/scalable/apps/ )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME ${PROJECT_ID}.svg )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
    COMMAND inkscape ${CMAKE_CURRENT_SOURCE_DIR}/logo/logo.svg -o ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/logo/logo.svg
)
add_custom_target(
    logo_png
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/${PROJECT_SLUG}.png DESTINATION share/icons/hicolor/512x512/apps )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/images/${PROJECT_SLUG}.svg DESTINATION share/icons/hicolor/512x512/apps/ RENAME ${PROJECT_ID}.png )

# Plugins
install(DIRECTORY plugins DESTINATION ${DATA_INSTALL})

function(plugin name)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins/${name}/plugin.json ${CMAKE_CURRENT_BINARY_DIR}/plugins/${name}/plugin.json COPYONLY)
    foreach(file ${ARGN})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins/${name}/${file} ${CMAKE_CURRENT_BINARY_DIR}/plugins/${name}/${file} COPYONLY)
    endforeach()
endfunction()

plugin(
    Hello
    hello_world.py
)

plugin(
    ReplaceColor
    replace_color.py
    replace.ui
)

plugin(
    AnimatedRaster
    animated_raster.py
)

plugin(
    dotLottie
    dotlottie.py
    icon.svg
)

plugin(
    Synfig
    synfig.py
)

plugin(
    HueShift
    hue_shift.py
)


plugin(
    FrameByFrame
    create_frames.py
    create_frames.ui
)
