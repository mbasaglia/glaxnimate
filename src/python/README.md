Python Glaxnimate
=================

Python bindings for [Glaxnimate](https://glaxnimate.mattbas.org/).

Allows to create and modify vector animations. With support for Lottie, SVG, and other formats.

See the [documentation page](https://glaxnimate.mattbas.org/contributing/scripting/) for more details.

## Examples

### Convert animated SVG to Lottie

```py
import glaxnimate

# Set up environment
with glaxnimate.environment.Headless():
    # Create a document object
    document = glaxnimate.model.Document("")

    # Load an animated SVG
    with open("MyFile.svg", "rb") as input_file:
        glaxnimate.io.registry.from_extension("svg").load(document, input_file.read())

    # ...

    # Write to Lottie
    with open("MyFile.json", "wb") as output_file:
        output_file.write(glaxnimate.io.registry.from_extension("json").save(document))
```


### Render Lottie to gif


```py
from PIL import Image
from PIL import features
import glaxnimate


def png_gif_prepare(image):
    """
    Converts the frame image from RGB to indexed, preserving transparency
    """
    if image.mode not in ["RGBA", "RGBa"]:
        image = image.convert("RGBA")
    alpha = image.getchannel("A")
    image = image.convert("RGB").convert('P', palette=Image.ADAPTIVE, colors=255)
    mask = Image.eval(alpha, lambda a: 255 if a <= 128 else 0)
    image.paste(255, mask=mask)
    return image


def save_gif(document, file, skip_frames=1):
    start = int(document.main.animation.first_frame)
    end = int(document.main.animation.last_frame)

    # Get all frames as PIL images
    frames = []
    for i in range(start, end+1, skip_frames):
        frames.append(png_gif_prepare(document.render_image(i)))

    # Save as animation
    duration = int(round(1000 / document.main.fps * skip_frames / 10)) * 10
    frames[0].save(
        file,
        format='GIF',
        append_images=frames[1:],
        save_all=True,
        duration=duration,
        loop=0,
        transparency=255,
        disposal=2,
    )

# Initialize environment
with glaxnimate.environment.Headless():

    document = glaxnimate.model.Document("")

    # Load the lottie JSON
    with open("MyFile.json", "rb") as input_file:
        glaxnimate.io.registry.from_extension("json").load(document, input_file.read())

    # Save as GIF
    with open("MyFile.gif", "rb") as output_file:
        save_gif(document, output_file)
```

### Create animations from code

```py
import glaxnimate

with glaxnimate.environment.Headless():
    # Create an empty document
    document = glaxnimate.model.Document("")

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

    # Export it
    with open("MyFile.svg", "wb") as output_file:
        output_file.write(glaxnimate.io.registry.from_extension("svg").save(document))
```

## Dependencies

This module depends on the following system libraries

* Qt5 (widgets, xml)
* potrace
* libav / ffmpeg
* libarchive

To install them on Ubuntu and similar:

```bash
apt install libqt5widgets5 libqt5xml5 potrace ffmpeg libarchive13
```
