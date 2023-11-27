# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

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

if(NOT ANDROID)
    add_compile_definitions(Q_OS_ANDROID_FAKE)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/external/QtAppSetup/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/external/QtColorWidgets/include
)

if (ANDROID)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS AndroidExtras Svg REQUIRED)
endif()

add_executable(${PROJECT_SLUG}_mobile ${SOURCES})
kde_target_enable_exceptions(${PROJECT_SLUG}_mobile PUBLIC)

target_link_libraries(${PROJECT_SLUG}_mobile PUBLIC
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
)

if (ANDROID)
    target_link_libraries(${PROJECT_SLUG}_mobile PUBLIC
        Qt${QT_VERSION_MAJOR}::AndroidExtras
        Qt${QT_VERSION_MAJOR}::Svg
    )
endif()

string(TIMESTAMP VERSION_CODE "%s" UTC)

#install(TARGETS ${PROJECT_SLUG}_mobile ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS ${PROJECT_SLUG}_mobile)

# Download Twemoji
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/emoji
    COMMAND rm -r -f /tmp/noto-emoji-main
    COMMAND git clone https://github.com/googlefonts/noto-emoji.git /tmp/noto-emoji-main
    COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/emoji
    COMMAND mv -T /tmp/noto-emoji-main/svg ${CMAKE_CURRENT_BINARY_DIR}/emoji/svg
    COMMAND mv -T /tmp/noto-emoji-main/png/72 ${CMAKE_CURRENT_BINARY_DIR}/emoji/png
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/copy_emoji_flags.py /tmp/noto-emoji-main/third_party/region-flags/svg/ ${CMAKE_CURRENT_BINARY_DIR}/emoji/svg
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/copy_emoji_flags.py /tmp/noto-emoji-main/third_party/region-flags/png/ ${CMAKE_CURRENT_BINARY_DIR}/emoji/png
)
add_custom_target(emoji_images
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/emoji
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/emoji DESTINATION ${DATA_INSTALL} PATTERN *@* EXCLUDE)
add_dependencies(${PROJECT_SLUG}_mobile emoji_images)
message(STATUS ${DATA_INSTALL}/emoji)

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


# # Installs (and runs the apk)
# add_custom_target(
#     android_install
#     COMMAND adb install -r $<TARGET_PROPERTY:${PROJECT_SLUG}_mobile,apk>
#     COMMAND adb shell am force-stop ${PACKAGE_NAME}
#     COMMAND adb shell am start -n ${PACKAGE_NAME}/.GlaxnimateActivity -W
#     #DEPENDS ${PROJECT_SLUG}_mobile
# )

# Attaches to the logs of the running app
add_custom_target(
    android_log
    COMMAND adb logcat --pid `adb shell ps -o PID,ARGS | grep -F ${PACKAGE_NAME} | sed -r "'s/^\\s*([0-9]+).*/\\1/'" `
)
