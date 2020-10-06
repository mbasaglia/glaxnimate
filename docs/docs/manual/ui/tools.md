Authors: Mattia Basaglia

# Tools

## Select

<img src="/img/ui/icons/edit-select.svg" width="64" />

This tool allows you to select items and move them.

### Mouse Actions

This tool has a multitude of mouse interactions:

Left click: Select the item under the mouse.

Dragging a selected item: Moves the item while you drag it.

Dragging a handle: Moves the handle, editing the associated property.

Dragging from an area without items: Creates a rectangular selection area,
when you release the mouse it will select all items intersecting that area.

Dragging while holding Alt: Draws a path and when you release the mouse,
it will select all items intersecting that path.

Right click: show a contect menu with actions regarding the current selection
and the item under the mouse.

Double click: edit the item under the mouse

All selection actions (click, draw drag, rectangle drag) will replace the current
selection by default, or add to the current selection if you are holding Shift while releasing the mouse button.

### Keyboard Actions

If you press backspace or delete you can delete the selected items.

### Context Menu

![Context Menu](/img/screenshots/tools/select_menu.png)

The context menu with the actions affecting the current selection.

At the bottom you get a submenu for the item under the mouse cursor:

![Shape Menu](/img/screenshots/tools/shape_menu.png)

## Edit Nodes

<img src="/img/ui/icons/edit-node.svg" width="64" />

This tool allows you to select and edit Shapes.

### Mouse Actions

Clicking on bezier nodes will select them (Shift+Click to add to the selection).

Clicking on shapes, will selected them and enable their editing.

Dragging from an empty spot will create a rectangular selection area to select bezier nodes.

Right clicking on the handles for a bezier node or its tangents will show a context menu

### Context Menu

![Context Menu](/img/screenshots/tools/menu_vertex.png)
![Context Menu](/img/screenshots/tools/menu_tangent.png)

The menu is slightly different depending on whether you clicked on a node or tangent.

The first few actions change the node type.

<img src="/img/ui/icons/show-node-handles.svg" width="32" /> *Remove Tangent*<br/>
Removes the selected tangent, making the curve exiting straight as it exits
the node in the tangent's direction.
If the tangent on the other side has also been removed, the segment between the
two nodes will be straight.

<img src="/img/ui/icons/format-remove-node.svg" width="32" /> *Remove Node*<br/>
Removes the selected node.

<img src="/img/ui/icons/show-node-handles.svg" width="32" /> *Show Tangents*<br/>
Shows the handles for removed tangents.

## Draw Bezier

<img src="/img/ui/icons/draw-bezier-curves.svg" width="64" />

This tool allows you to create [Bezier shapes](../shapes.md#path).

It has the [common shape options](#shape-options).

### Mouse Actions

Clicking or dragging will enter drawing mode, each subsequent click will add a new point to the shape being created.

Clicking on the starting point will close the curve, create the new shape and exit drawing mode.

Double clicking will add a new point, create the new shape and exit drawing mode.

Dragging will pull on the handles of the last added point. <br/>
By default the point will be symmetrical, this means the input and output tangent will have the same size and direction.<br/>
If you hold Ctrl while dragging, the point will become smooth,
this means the direction of the tangents will stay the same but they can have different lenths.<br/>
If you hold Shift instead, it will become a corner: the in tangent will not be modified any more
and you can change the out tangent as you please.

Releasing Ctrl or Shift will revert the point back to symmetrical.

#### The different tangent types

![Symmetrical](/img/screenshots/tools/bez_drag_sym.png)

![Smooth](/img/screenshots/tools/bez_drag_smooth.png)

![Corner](/img/screenshots/tools/bez_drag_corn.png)

### Keyboard Actions

Pressing backspace or delete will remove the last point you added.

Pressing enter will create the shape and exit drawing mode.

Pressing escape will remove the unfinished shape and exit drawing mode.

## Rectangle

<img src="/img/ui/icons/draw-rectangle.svg" width="64" />

This tool allows you to create [rectangles and squares](../shapes.md#rectangle).

It has the [common shape options](#shape-options).

### Mouse Interactions

You click drag with the left mouse button.
The position you pressed the button on will be a corner of the rectangle
and the position where you release the button will be the opposite corner.

If you hold Ctrl while dragging, the rectangle will be a square (all sides with equal length).

If you hold Shift, the first point will be interpreted as the center of the rectangle rather than a corner.

Clicking (without dragging) on an existing object will switch to the edit tool.

### Keyboard Interactions

Pressing Escape will cancel the rectangle currently being dragged.

## Ellipse

<img src="/img/ui/icons/draw-ellipse.svg" width="64" />

This tool allows you to create [ellipses and circles](../shapes.md#ellipse).

It works the same way as the [rectangle tool](#rectangle).

## Star / Polygon

<img src="/img/ui/icons/draw-polygon-star.svg" width="64" />

This tool allows you to create [stars and regular polygons](../shapes.md#polystar).

### Mouse Interactions

You click and drag, the point where you initiated the drag defines the center
and the point you release the drag defines one of the corners.

Clicking (without dragging) on an existing object will switch to the edit tool.

### Keyboard Interactions

Pressing Escape will cancel the shape currently being dragged.

### Options

![Star Options](/img/screenshots/tools/star_options.png)

The combo box at the top allows you to toggle between star and regular polygon mode.

**Spoke Ratio** determines the ratio between the outer and inner radius for stars.<br/>
The value goes between 0 and 1, the closer to 0 the pointier the star will look.

**Corners** is the number of corners for the shape
(eg: 5 will produce a 5-pointed star or a pentagon).

The other options are the [common shape options](#shape-options).

## Shape Options

![Shape Options](/img/screenshots/tools/shape_options.png)

These options allow you to select what gets created when you finish drawing the shape.

* Group: Will add a shape group containing the shape. Disabling this will disable the other two.
* Stroke: Will create a stroke shape. See the [stroke style view](docks.md#stroke) for information on how to change the stroke style.
* Fill: Will create a fill shape. See the [fill style view](docks.md#fill) for information on how to change the stroke style.


## Color Picker

<img src="/img/ui/icons/color-picker.svg" width="64" />

Selects a color from the screen.


### Mouse Interactions

Moving the mouse on the canvas will show the color being picked.
Clicking assigns the color to the selected shape.

If you click and drag, you will be able to select color outside the canvas.

### Options

![Color Picker Options](/img/screenshots/tools/picker_options.png)

* *Fill* When clicking, sets the fill color
* *Stroke* When clicking, sets the stroke color

Holding *Shift* will toggle the two.
