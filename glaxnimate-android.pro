QT += widgets xml uitools concurrent
# requires(qtConfig(listview))

CONFIG += c++17

INCLUDEPATH += src/core src/gui external/QtAppSetup/src build/bin/core external/Qt-Color-Widgets/include

LIBS += -lz

TEMPLATE = subdirs

SUBDIRS = \
    color_widgets \
    external/QtAppSetup \
    external/potrace \
    src/core \
    src/gui \
    src/android \
#    history_lineedit \

color_widgets.file = external/Qt-Color-Widgets/color_widgets.pro

external/QtAppSetup.depends = color_widgets
src/core.depends = external/QtAppSetup external/potrace
src/gui.depends = src/core
src/android.depends = src/gui

CONFIG += ordered

android {
    assets_icons.files = data/icons/breeze-icons/icons/
    assets_icons.path = /assets/icons/
    assets_images.files = data/images
    assets_images.path = /assets/
    INSTALLS += assets_icons assets_images
}
