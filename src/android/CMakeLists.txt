set(SOURCES
    android_file_picker.cpp
    android_intent_handler.cpp
    android_style.cpp
    base_dialog.cpp
    better_toolbox_widget.cpp
    document_opener.cpp
    emoji_data.cpp
    emoji_widget.cpp
    format_selection_dialog.cpp
    help_dialog.cpp
    main.cpp
    main_window.cpp
    main_window.ui
    scroll_area_event_filter.cpp
    sticker_pack_builder_dialog.cpp
    sticker_pack_builder_dialog.ui
    telegram_intent.cpp
    timeline_slider.cpp
)


include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/external/QtColorWidgets/include)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS AndroidExtras REQUIRED)

add_library(${PROJECT_SLUG} SHARED ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC
    QtAppSetup
    ${LIB_NAME_CORE}
    ${LIB_NAME_GUI}
    QtColorWidgets
    #QtHistoryLineEdit
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::UiTools
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::AndroidExtras
)

include(qt-android-cmake/AddQtAndroidApk.cmake)

add_qt_android_apk(
    "${PROJECT_SLUG}_apk"
    ${PROJECT_SLUG}
    NAME ${PROJECT_NAME}
    VERSION_CODE 1
    PACKAGE_NAME "org.mattbas.glaxnimate"
    PACKAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/android
    #DEPENDS
        #QtColorWidgets
        #QtHistoryLineEdit
        #Qt${QT_VERSION_MAJOR}::Core
        #Qt${QT_VERSION_MAJOR}::Gui
        #Qt${QT_VERSION_MAJOR}::Widgets
        #Qt${QT_VERSION_MAJOR}::UiTools
        #Qt${QT_VERSION_MAJOR}::Concurrent
    #INSTALL
)


message(STATUS "+++++++")
message(STATUS ${ANDROID_TOOLCHAIN_ROOT})
