Authors: Mattia Basaglia

# Scripting

Glaxnimate provides an extensive scripting interface.

See full [API Reference](python_reference.md) page to see all the available modules, classes, and functions.

There are also some useful [examples](examples.md) that walk you through a few features.

## Script Console

You can use the [Script Console](/manual/ui/docks#script-console) to test some
snippets and modify the current document.

Here you can access the `glaxnimate` module and you have the global objects
`window` and `document` defined.

The console has a button to reload the script module, this means that if you make changes
to a python module / script and want the change to take effect, you can click it and you'll
see the up to date modules (instead of the version previously cached by Python).

This also affects modules loaded for plugins.

If you want to run more complex scripts in the console, you can use [Snippets](/manual/ui/docks#snippets).

## Plugins

You can write plugins to perform common actions, add support for more file formats, and more.

See [Plugins](/contributing/scripting/plugins) for a full description of the structure of a plugin.

If you are developing a plugin, the easiest way to reload the modules when you make changes, is to
click on *Reload Script Modules* in the script console.

## As a Python module

Glaxnimate is also available as a mode (you can download it from [PyPI](https://pypi.org/project/glaxnimate/)).

```
pip install glaxnimate
```

You can also download the development version (This will have newer features but might also have more bugs_

```
pip install glaxnimate --extra-index-url https://gitlab.com/api/v4/projects/19921167/packages/pypi/simple
```

It provides a similar functionality as the script console but you can call it from python scripts, without the need
of a GUI.

Some functionality needs a Glaxnimate environment to run but that's easy to set up:

```python

import glaxnimate

# Set up environment
with glaxnimate.environment.Headless():
    # Create a document object
    document = glaxnimate.model.Document("")

    # Load a file
    with open("MyFile.rawr", "rb") as input_file:
        glaxnimate.io.registry.from_extension("rawr").load(document, input_file.read())


    # Add a layer
    layer = document.main.add_shape("Layer")

    # The fill will be applied to all following shapes in the same group / layer
    fill = layer.add_shape("Fill")
    fill.color.value = "#ff0000"

    # A simple circle moving left and right
    ellipse = layer.add_shape("Ellipse")
    radius = 64
    ellipse.position.set_keyframe(0, glaxnimate.utils.Point(radius, document.size.height / 2))
    ellipse.position.set_keyframe(90, glaxnimate.utils.Point(document.size.width-radius, document.size.height / 2))
    ellipse.position.set_keyframe(180, glaxnimate.utils.Point(radius, document.size.height / 2))
    ellipse.size.value = glaxnimate.utils.Size(radius, radius)

    # Write a file
    with open("MyFile.json", "rb") as output_file:
        output_file.write(glaxnimate.io.registry.from_extension("json").save(document))
```
