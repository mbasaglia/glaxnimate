Authors: Mattia Basaglia

# Introduction

## User Interface

![Main Window](/img/screenshots/main_window.png)

Glaxnimate user interface has the following components:

* The [canvas](ui/canvas.md) is the area in the middle of the window, where you
  can preview and edit the animation.
* Around the canvas are the [dockable views](ui/docks.md), that give quick access
  to all the main functionality of Glaxnimate.
  These can be hidden and re-arranged to fit your taste.
* On top are the [menu and tool bars](ui/menus.md), these work like in most user interfaces.

You can click on the links to the various pages to get more details on each interface component.


## Core Concepts

### Vector graphics

Glaxnimate works with vector graphics, this means images are described with
objects like lines, curves, and points. This is different from the more common
raster graphics where you have a grid of pixels of different colors.

An advantage of using vector graphics is that you can view the image at any
resolution without losing quality.

### Tweening

When animating vector graphics, you have the option of automatically generating
smooth transitions between poses, in the process known as "Tweening" (or Inbetweening).

The term comes from the action of adding frames in between two "key" frames
that define the start and end of the animation.

Glaxnimate allows you to do just this: you specify shapes and properties
for each keyframe and the animation is automatically created from those.

### Further reading

* [Inbetweening](https://en.wikipedia.org/wiki/Inbetweening)
* [Vector Graphics](https://en.wikipedia.org/wiki/Vector_graphics)
