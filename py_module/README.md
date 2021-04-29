Python Glaxnimate
=================

Python bindings for [Glaxnimate](https://glaxnimate.mattbas.org/).

Allows to create and modify vector animations. With support for Lottie, SVG, and other formats.

See the [documentation page](https://glaxnimate.mattbas.org/contributing/scripting/) for more details.

## Example

```py
import glaxnimate

# Set up environment
with glaxnimate.environment.Headless():
    # Create a document object
    document = glaxnimate.model.Document("")

    # Load an animated SVG
    with open("MyFile.svg", "rb") as input_file:
        glaxnimate.io.registry.from_extension("svg").load(document, input_file)

    # ...

    # Write to Lottie
    with open("MyFile.json", "rb") as output_file:
        output_file.write(glaxnimate.io.registry.from_extension("json").save(document))
```
