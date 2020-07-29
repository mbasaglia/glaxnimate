set(SOURCES

app/application.cpp
app/settings/settings.cpp
app/translation_service.cpp
app/scripting/plugin.cpp
app/widgets/plugin_settings_widget.cpp
app/widgets/plugin_settings_widget.ui
app/widgets/settings_dialog.cpp
app/widgets/settings_dialog.ui
)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
if ( ${WTF_IS_THIS_WARNING} )
    SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
endif()

add_library(${PROJECT_SLUG} OBJECT ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt5::Widgets QtColorWidgets QtHistoryLineEdit)
target_include_directories(${PROJECT_SLUG} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
