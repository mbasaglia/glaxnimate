Authors: Mattia Basaglia

# Scripting

Glaxnimate provides an extensive scripting interface.

See full [API Reference](python_reference.md) page to see all the available modules, classes, and functions.

There are also some useful [examples](examples.md) that walk you through a few features.

## Script Console

You can use the [Script Console](/manual/ui/docks.md#script-console) to test some
snippets and modify the current document.

Here you can access the `glaxnimate` module and you have the global objects
`window` and `document` defined.

## Plugins

You can write plugins to perform common actions, add support for more file formats, and more.

See [Plugins](/contributing/scripting/plugins.md) for a full description of the structure of a plugin.

## As a Python module

Glaxnimate is also available as a mode (you can download it from [PyPI](https://pypi.org/project/glaxnimate/)).

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
        glaxnimate.io.registry.from_extension("rawr").load(document, input_file)

    # ...

    # Write a file
    with open("MyFile.json", "rb") as output_file:
        output_file.write(glaxnimate.io.registry.from_extension("json").save(document))
```
