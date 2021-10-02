Authors: Mattia Basaglia

# Translations

## Adding Languages

To add a new translation, edit the file `data/CMakeLists.txt` to add
the translation file in the form `translations/glaxnimate_(code).ts`,
where *(code)* is a locale code (eg: **en**, **en_US**, ...).

Then run `make translations` for the file to be generated.

## Editing Translations

To edit a translation file, you can open it with Qt Linguist, which should
show context information and the string in the GUI when needed.

The translation files are under `data/translations`.

After a file has been edited, `make translations` will build the output file
and the translation should be available in Glaxnimate settings.


## Status

{translation_table}
