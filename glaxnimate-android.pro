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
#    assets.files = data/icons/breeze-icons/icons/actions/22/document-open.svg
#    assets.path = /assets/icons
    assets.files = data/icons/breeze-icons/icons/
    assets.path = /assets/icons/
    INSTALLS += assets
}
