Authors: Mattia Basaglia

# Examples

## Showing a Dialog

Sometimes a plugin might need to show a dialog to the user.

Todo so, you can create a UI file using [Qt Designer](https://doc.qt.io/qt-5/qtdesigner-manual.html).

You can then load that file and display the dialog through the `window` object:


```python
# Assume this is the function being invoked by the plugin
def plugin_main(window, document, settings):

    # Load the dialog from myform.ui
    dialog = window.create_dialog("myform.ui")

    # If there's an error, it will be logged
    if not dialog:
        return

    # Sets the `text` property of the widget called `my_label`.
    # The widget names are the ones you have in the UI file,
    # for the available properties refer to the Qt documentation
    dialog.set_value("my_label", "text", "Hello world")

    # It might be useful to set the values for some of the widget based on the
    # window or selection
    dialog.set_value("color_selector", "color", window.current_color)

    # `exec()` will show the dialog and block until the user accepts it or closes it.
    # It returns 1 if the dialog was accepted, 0 if it was canceled by the user
    if dialog.exec():
        # `get_value()` is analogous to `set_value()`
        value = dialog.get_value("color_selector", "color")

        # ...
```

## Document Visitor

This example shows how to have a plugin inspect all objects in the document and
change some that match a given condition.

The code is taken from the `ReplaceColor` plugin.

```python
import glaxnimate

# Subclass the Visitor class, it will automatically go through all
# the visual elements of the document
class Replacer(glaxnimate.model.Visitor):
    def __init__(self, color_search, color_replace):
        # Always call the parent constructor
        super().__init__()
        self.color_search = color_search
        self.color_replace = color_replace

    # This gets called on every node of the document
    def on_visit_node(self, node):
        if (
            # Here we want to change colors, so we are only interested in Styler objects
            # (These are Fill and Stroke)
            isinstance(node, glaxnimate.model.shapes.Styler) and
            # Also check that the current color matches the one we are looking for
            node.color.value == self.color_search
        ):
            # Update the color
            node.color.value = self.color_replace
```

You can invoke the visitor using the `visit` function:

```python
    # With the visitor is always useful to have a macro active,
    # so whole operation will be seen as a single undoable operation
    with document.macro("Replace color"):
        # The second parameter to `visit` is True because we want to skip
        # locked objects.
        Replacer(search_color, replace_color).visit(document, True)
```
