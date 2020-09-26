Authors: Mattia Basaglia

# Repository Structure

This page describes the layout of source file in the repository.

## cmake

This is a git submodule which contains some reusable CMake scripts

## data

Here goes all data and assets (eg: translations, images, etc)

### data/icons

Icon theme (from [Breeze](https://github.com/KDE/breeze-icons)).

### data/images

Includes custom icons (ones not found in the icon theme), logos, and other images.

### data/plugins

To contain default plugins. See also [Writing Plugins](scripting/plugins.md).

### data/translations

Translation source files, see [Translations](index.md#translations) for details.

## docs

Contains documentation files, see [documentation](documentation.md) for details.

## external

Contains code for libraries not strictly part of Glaxnimate itself.

## src

Main codebase for Glaxnimate.

### src/core

Contains the internal logic for handling documents, animations, saving etc.
Basically everything in Glaxnimate that doesn't relate to the graphical interface.

#### src/core/command

Undo/Redo commands.

#### src/core/io

Importers and Exporters.

#### src/core/math

Miscellaneous mathematical utilities.

#### src/core/model

Classes that define the document structure.

#### src/core/utils

Miscellaneous programming utilities.

### src/gui

Code for the graphical user interface.

The GUI uses [Qt](https://doc.qt.io/) extensively.

#### src/gui/graphics

Elements relating to the main canvas.

#### src/gui/item_models

Models for the [Qt Model/View](https://doc.qt.io/qt-5/model-view-programming.html) framework.

#### src/gui/style

Widget styling classes.

#### src/gui/tools

Editor tools, they define how the user can interact with the canvas.

#### src/gui/widgets

All classes derived from QWidget.

##### src/gui/widgets/dialogs

Code for dialogs and windows.
All widgets that are displayed on their own (ie: not as part of some other widget).

## test

Unit tests.
