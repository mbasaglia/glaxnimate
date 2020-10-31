Authors: Mattia Basaglia

# Examples

## Undo Macros

Every time you change a property in python, a new undo command will be created.

This means that from Glaxnimate you can undo/redo every single operation by itself.

```python
# After the following two lines you'll see "Update width" and "Update height"
# in the Undo  History view. Each can be undone individually
document.main.width = 100
document.main.height = 100
```

For plugins it makes more sense if all changes are grouped under a single command.
To achieve this you can create macros:

```python
# This will create a single undo command called "Resize".
# Undoing it will revert both width and height to their respective values.
with document.macro("Resize"):
    document.main.width = 100
    document.main.height = 100
```

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

## Rendering still images

You can render still frames as PIL/Pillow Image objects using `Document.render_image`.

Without any arguments it will return the image with the current frame
(ie: what is being displayed in the GUI).

```python
document.render_image()
```

It has two optional arguments: the first selects the time to render (in frames, can also be fractional),
the second is the size of the image you want.

```python
# These two are equivalent
document.render_image(10)
document.render_image(10, document.size())

# Custom size, everything will be scaled to fit
document.render_image(10, glaxnimate.utils.IntSize(10, 10))
```

## Creating shapes

You can use `add_shape` in Groups, Layers, and Compositions to create new shapes.

You'll have to create the shape first and then populate its property.

You specify the type of the shape or element you want to create by its name (without module).

```python
# Create a new group
group = document.main.add_shape("Group")

# Add a rect at the end
rect = group.add_shape("Rect")
rect.size.value = glaxnimate.utils.Size(100, 50)
rect.position.value = document.size.to_point() / 2

# Create a fill color at index 0
group.add_shape("Fill", 0).color.value = "#0000ff"

# Select the created group
window.current_item = rect
```

## Showing messages to the user

There are several ways of displaying messages, depending on what you need:

`window.status` Will show a temporary message on the status bar, this is to show progress and such

```python
window.status("Message")
```

`window.warning` Will show a large warning message, this is for user-facing error reporting.
It will also show a warning in the Logs view.

```python
window.warning("Message")
```

`glaxnimate.log` Will log messages in the Logs view. Use this for internal reporting.

```python
glaxnimate.log.warning("Message")
glaxnimate.log.info("Message")
glaxnimate.log.error("Message")
```
