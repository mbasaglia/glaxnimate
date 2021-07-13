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

add_library(${PROJECT_SLUG}_android SHARED ${SOURCES})
target_link_libraries(${PROJECT_SLUG}_android PUBLIC
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

set(ANDROID_PLATFORM 29)
set(ANDROID_PLATFORM_LEVEL ${ANDROID_PLATFORM})
set(ANDROID_SDK "$ENV{HOME}/Android/Sdk" CACHE STRING "")
set(PACKAGE_NAME "org.mattbas.glaxnimate")
string(TIMESTAMP VERSION_CODE "%s" UTC)

add_qt_android_apk(
    ${PROJECT_SLUG}_apk
    ${PROJECT_SLUG}_android
    NAME ${PROJECT_NAME}
    VERSION_CODE ${VERSION_CODE}
    PACKAGE_NAME ${PACKAGE_NAME}
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

add_custom_target(
    android_avd_list
    COMMAND "${ANDROID_SDK}/emulator/emulator" -list-avds
)

add_custom_target(
    android_avd_start
    COMMAND "${ANDROID_SDK}/emulator/emulator" -avd "$$DEVICE"
)

add_custom_target(
    android_install
    COMMAND adb install -r $<TARGET_PROPERTY:${PROJECT_SLUG}_apk,apk>
    COMMAND adb shell am force-stop ${PACKAGE_NAME}
    COMMAND adb shell am start -n ${PACKAGE_NAME}/.GlaxnimateActivity
    DEPENDS ${PROJECT_SLUG}_apk
)
