Authors: Mattia Basaglia

# Dialogs

## About

Shows various information about Glaxnimate.

### Credits

![About dialog](/img/screenshots/dialogs/about_credits.png)

General information and links.

### Paths

![About dialog](/img/screenshots/dialogs/about_paths.png)

Here you can see and open the settings file, and all paths where Glaxnimate looks for data.

### System

![About dialog](/img/screenshots/dialogs/about_system.png)

Shows the system information. You can click on "Copy" to copy them to the clipboard.

## Move To

![Move To dialog](/img/screenshots/dialogs/move_to.png)

Aloows you to choose a destination to move the selected shapes.

## Resize

![Resize dialog](/img/screenshots/dialogs/resize.png)

Allows to resize the animation and scale all layers.

## Timing

![Timing dialog](/img/screenshots/dialogs/timing.png)

Allows you to change framerate and duration.

## Trace Bitmap

This dialog allows you to convert raster images (bitmaps) into vector paths.

### Closest Color

![Trace dialog](/img/screenshots/dialogs/trace/closest.png)

In the *Closest Color* mode, you can select a set of colors and they will
be used to determine how shapes are defined.

This algorithm works by using the color on the list that most closely matches
the pixel values, and pixels with the same resulting color will end up in the same shape.

You can edit the list of colors by using the buttons to add or remove colors,
then you can click on the list to change the color.

Alternatively, you can click on *Detect Colors* which will fill the list with
the most common colors in the image (you select how many colors you want with
the spin box next to the button.

### Extract Colors

![Trace dialog](/img/screenshots/dialogs/trace/extract.png)

This mode has the same image options as *Closest Color* but it will only
use pixels that match the specified colors exactly.

The difference is more obvious when you have fewer colors; in the following
example the two modes are operating on the same image with the same colors:

![Trace dialog](/img/screenshots/dialogs/trace/extract_fewer.png)

![Trace dialog](/img/screenshots/dialogs/trace/closest_fewer.png)

As you can see, *Extract Colors* only used the parts of the image that had matching
colors, resulting in fewer shapes being extracted; while *Closest Color* assigned
the best available color for each area.

Generally, *Closest Color* is the best choice when you want to trace the whole image,
*Extract Colors* when you only need specific elements.

### Transparency

![Trace dialog](/img/screenshots/dialogs/trace/alpha.png)

This mode ignore colors and just looks at the transparency (Alpha channel) of the image.

It has the following options:

* Output Color - This is the color of the shapes you get
* Invert - Will generate the inverse image: shapes will occupy the transparent areas
* Threshold - Alpha value below which a pixel is considered transparent

This mode is best suited for converting monochrome logos and the like.

### Curve Options

These options affect the appearance of the output shapes

* Smoothness - A value of 0% will create polygons, a value of 100% will avoid any sharp corners
* Minimum Area - If a region with the same color doesn't have more than these many pixels, it will be discarder. Useful to remove speckles.

