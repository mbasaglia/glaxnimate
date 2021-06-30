
OBJECTS_DIR = out/obj
MOC_DIR = out/generated
UI_DIR = out/generated
RCC_DIR = out/generated

android {
    OBJECTS_DIR = $$ANDROID_TARGET_ARCH/out/obj
    MOC_DIR = $$ANDROID_TARGET_ARCH/out/generated
    UI_DIR = $$ANDROID_TARGET_ARCH/out/generated
    RCC_DIR = $$ANDROID_TARGET_ARCH/out/generated
}

QT += widgets

INCLUDEPATH += $$PWD/src/ $$PWD/../Qt-Color-Widgets/include

CONFIG += c++17

TEMPLATE=lib

LIBS += -L../Qt-Color-Widgets -lQtColorWidgets

SOURCES = \
src/app/cli.cpp \
src/app/settings/palette_settings.cpp \
src/app/settings/settings.cpp \
src/app/settings/keyboard_shortcuts_model.cpp \
src/app/settings/keyboard_shortcuts.cpp \
src/app/scripting/script_engine.cpp \
src/app/widgets/widget_palette_editor.cpp \
src/app/widgets/keyboard_settings_widget.cpp \
src/app/widgets/clearable_keysequence_edit.cpp \
src/app/widgets/settings_dialog.cpp \
src/app/log/log_model.cpp \
src/app/log/logger.cpp \
src/app/translation_service.cpp \
src/app/application.cpp

HEADERS = \
src/app/application.hpp \
src/app/settings/setting_group.hpp \
src/app/settings/keyboard_shortcuts.hpp \
src/app/settings/widget.hpp \
src/app/settings/keyboard_shortcuts_model.hpp \
src/app/settings/settings.hpp \
src/app/settings/widget_builder.hpp \
src/app/settings/palette_settings.hpp \
src/app/settings/setting.hpp \
src/app/settings/custom_settings_group.hpp \
src/app/translation_service.hpp \
src/app/utils/qstring_hash.hpp \
src/app/utils/desktop.hpp \
src/app/env.hpp \
src/app/debug/model.hpp \
src/app/scripting/script_engine.hpp \
src/app/widgets/settings_dialog.hpp \
src/app/widgets/keyboard_settings_widget.hpp \
src/app/widgets/clearable_keysequence_edit.hpp \
src/app/widgets/widget_palette_editor.hpp \
src/app/widgets/no_close_on_enter.hpp \
src/app/cli.hpp \
src/app/log/listener_stderr.hpp \
src/app/log/log_model.hpp \
src/app/log/log_line.hpp \
src/app/log/logger.hpp \
src/app/log/log.hpp \
src/app/log/listener_store.hpp \
src/app/qstring_exception.hpp

FORMS = \
src/app/widgets/keyboard_settings_widget.ui \
src/app/widgets/settings_dialog.ui \
src/app/widgets/widget_palette_editor.ui \
src/app/widgets/clearable_keysequence_edit.ui
