set(SOURCES
    android_file_picker.cpp
    android_intent_handler.cpp
    android_style.cpp
    base_dialog.cpp
    better_toolbox_widget.cpp
    document_opener.cpp
    format_selection_dialog.cpp
    help_dialog.cpp
    main.cpp
    main_window.cpp
    main_window.ui
    sticker_pack_builder_dialog.cpp
    sticker_pack_builder_dialog.ui
    telegram_intent.cpp
    timeline_slider.cpp
)


include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/external/QtColorWidgets/include)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS AndroidExtras Svg REQUIRED)

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
    Qt${QT_VERSION_MAJOR}::Svg
)

set(ANDROID_PLATFORM_LEVEL ${ANDROID_PLATFORM})
set(ANDROID_SDK "$ENV{HOME}/Android/Sdk" CACHE STRING "")
set(PACKAGE_NAME "org.mattbas.glaxnimate")
set(JAVA_HOME "/usr/lib/jvm/default-java" CACHE STRING "")
string(TIMESTAMP VERSION_CODE "%s" UTC)

include(qt-android-cmake/AddQtAndroidApk.cmake)
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

# Download Twemoji
get_target_property(ASSETS_PATH ${PROJECT_SLUG}_apk assets)
add_custom_command(
    OUTPUT ${ASSETS_PATH}/emoji
    COMMAND wget "https://github.com/googlefonts/noto-emoji/archive/refs/heads/main.tar.gz" -O /tmp/noto-emoji.tar.gz
    COMMAND tar -xaf /tmp/noto-emoji.tar.gz -C /tmp noto-emoji-main/svg noto-emoji-main/png/72 noto-emoji-main/third_party/region-flags
    COMMAND mkdir ${ASSETS_PATH}/emoji
    COMMAND mv -T /tmp/noto-emoji-main/svg ${ASSETS_PATH}/emoji/svg
    COMMAND mv -T /tmp/noto-emoji-main/png/72 ${ASSETS_PATH}/emoji/png
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/copy_emoji_flags.py /tmp/noto-emoji-main/third_party/region-flags/svg/ ${ASSETS_PATH}/emoji/svg
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/copy_emoji_flags.py /tmp/noto-emoji-main/third_party/region-flags/png/ ${ASSETS_PATH}/emoji/png
)
add_custom_target(emoji_images
    DEPENDS ${ASSETS_PATH}/emoji
)
add_dependencies(${PROJECT_SLUG}_apk emoji_images)
message(STATUS ${ASSETS_PATH}/emoji)

# Lists virtual devices
add_custom_target(
    android_avd_list
    COMMAND "${ANDROID_SDK}/emulator/emulator" -list-avds
)

# Starts a virtual device
# Usage:
#   make android_avd_start DEVICE="device"
# where "device" is from `android_avd_list`
add_custom_target(
    android_avd_start
    COMMAND "${ANDROID_SDK}/emulator/emulator" -avd "$$DEVICE" &
)

# Same as android_avd_start but starts from a clean state
add_custom_target(
    android_avd_clean_start
    COMMAND "${ANDROID_SDK}/emulator/emulator" -avd "$$DEVICE" -no-snapshot &
)


# Installs (and runs the apk)
add_custom_target(
    android_install
    COMMAND adb install -r $<TARGET_PROPERTY:${PROJECT_SLUG}_apk,apk>
    COMMAND adb shell am force-stop ${PACKAGE_NAME}
    COMMAND adb shell am start -n ${PACKAGE_NAME}/.GlaxnimateActivity -W
    #DEPENDS ${PROJECT_SLUG}_apk
)

# Attaches to the logs of the running app
add_custom_target(
    android_log
    COMMAND adb logcat --pid `adb shell ps -o PID,ARGS | grep -F ${PACKAGE_NAME} | sed -r "'s/^\\s*([0-9]+).*/\\1/'" `
)
