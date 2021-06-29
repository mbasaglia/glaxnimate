set(SOURCES
main.cpp
)


include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)

add_library(${PROJECT_SLUG} SHARED ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC
    #QtAppSetup
    #${LIB_NAME_CORE}
    #QtColorWidgets
    #QtHistoryLineEdit
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::UiTools
    Qt${QT_VERSION_MAJOR}::Concurrent
)

include(qt-android-cmake/AddQtAndroidApk.cmake)

add_qt_android_apk(
    "${PROJECT_SLUG}_apk"
    ${PROJECT_SLUG}
    NAME ${PROJECT_NAME}
    VERSION_CODE 1
    PACKAGE_NAME "org.mattbas.glaxnimate"
    #DEPENDS
        #QtColorWidgets
        #QtHistoryLineEdit
        #Qt${QT_VERSION_MAJOR}::Core
        #Qt${QT_VERSION_MAJOR}::Gui
        #Qt${QT_VERSION_MAJOR}::Widgets
        #Qt${QT_VERSION_MAJOR}::UiTools
        #Qt${QT_VERSION_MAJOR}::Concurrent
    INSTALL
)
