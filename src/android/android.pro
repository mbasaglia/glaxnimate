
OBJECTS_DIR = out/obj
MOC_DIR = out/generated
UI_DIR = out/generated
RCC_DIR = out/generated

QT += widgets xml uitools concurrent svg

android {
    QT += androidextras
    OBJECTS_DIR = $$ANDROID_TARGET_ARCH/out/obj
    MOC_DIR = $$ANDROID_TARGET_ARCH/out/generated
    UI_DIR = $$ANDROID_TARGET_ARCH/out/generated
    RCC_DIR = $$ANDROID_TARGET_ARCH/out/generated
}


!android {
    DEFINES += Q_OS_ANDROID_FAKE
}

TARGET = glaxnimate_android


CONFIG += c++17

INCLUDEPATH += $$PWD/../core $$PWD/../gui $$PWD/../../external/QtAppSetup/src $$PWD/../../external/Qt-Color-Widgets/include

LIBS += -L../../external/Qt-Color-Widgets -L../core -L../gui -L../../external/QtAppSetup -lglaxnimate_gui -lglaxnimate_core -lQtAppSetup -lQtColorWidgets -lz

!android {
    DEFINES += Q_OS_ANDROID
}

SOURCES = \
    android_file_picker.cpp \
    base_dialog.cpp \
    emoji_widget.cpp \
    format_selection_dialog.cpp \
    main.cpp \
    main_window.cpp \
    glaxnimate_app_android.cpp \
    telegram_intent.cpp \
    emoji_data.cpp

FORMS += \
    main_window.ui

HEADERS += \
    android_file_picker.hpp \
    android_style.hpp \
    base_dialog.hpp \
    emoji_data.hpp \
    emoji_widget.hpp \
    format_selection_dialog.hpp \
    glaxnimate_app_android.hpp \
    main_window.hpp \
    telegram_intent.hpp

RESOURCES += \
    resources.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/src/org/mattbas/glaxnimate/jnimessenger/JniMessenger.java

ANDROID_PACKAGE_SOURCE_DIR = \
    $$PWD/android
