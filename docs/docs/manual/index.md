Authors: Mattia Basaglia

# Introduction


## User Interface

![Main Window](/img/screenshots/main_window/main_window.png)

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

Glaxnimate works with vector graphics, which are images made up of
objects like lines, curves, and points. This is different from the more common
raster graphics where you have a grid of pixels of different colors.

The use of vector graphics allows Glaxnimate to provide a high level of
precision, scalability, and flexibility when it comes to creating and editing animations.
Because vector graphics are resolution-independent, animations created
in Glaxnimate can be easily resized or transformed without losing quality or clarity.

You can learn more about this in the [Vector Graphics](https://en.wikipedia.org/wiki/Vector_graphics)
article on Wikipedia.

### Tweening

When animating vector graphics, you have the option of automatically generating
smooth transitions between poses, in the process known as "Tweening" (or Inbetweening).

The term comes from the action of adding frames in between two "key" frames
that define the start and end of the animation.

Glaxnimate allows you to do just this: you specify shapes and properties
for each keyframe and the animation is automatically created from those.

You can learn more about this technique in the [Inbetweening](https://en.wikipedia.org/wiki/Inbetweening)
article on Wikipedia.

### Layers

Layers are used to group shapes and objects to have a more organized layout in a file.

Glaxnimate supports having multiple layers and layers nested inside other layers,
giving flexibility on how the file is structured.

You can easily convert between layers and groups, the main difference is that
groups are considered individual objects while layers aren't.

You can also read the manual page on [Groups and Layers](shapes.md#group) for a
more in-depth explanation.

### Precompositions

Precompositions are animations within another animation.

You can use it to animate an element once, and then make it appear in multiple
places using [Precomposition Layers](shapes.md#precomposition-layer).

When you modify the precomposition, the changes are reflected to all layers that
point to that composition so you don't have to apply the changed to every instance.

With precompositions you can also change when the animation starts and its duration.
This gives you the ability of creating elements that have looping animations simply
by creating multiple precomposition layers with different start times.
