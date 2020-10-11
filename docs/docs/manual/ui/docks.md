Authors: Mattia Basaglia

# Dockable Views


Tools
-----

![Tools View](/img/screenshots/views/tools.png)

This view allows you to select the active [tool](tools.md).

See [Tools](tools.md) for details on how each tool works.


Tool Options
------------

![Shape Options](/img/screenshots/tools/shape_options.png)

This view shows extra option for the currently active [tool](tools.md).

See [Tools](tools.md) for details on how each tool works.

Fill
----

![Colors View](/img/screenshots/colors/hsv.png)

This view allows to select / modify the current color.

The top part has different tabs to change which color space to use (or to use palettes).

The bottom part stays the same:

![Colors View Bottom](/img/screenshots/colors/bottom.png)

The "A" slider changes the color opacity (Alpha).

The two color indicators show the main (fill) color and secondary (stroke) color.<br/>
Clicking on either of them shows a dialog which gives similar controls as the ones in
the view, but more compact. And also allows picking colors from the screen.

The *Swap* button <img src="/img/ui/icons/swap-panels.svg" width="32" /> swaps the two colors.

Finally, there's a textbox with a hexadecimal representation of the main color.

### HSV

![HSV View](/img/screenshots/colors/hsv.png)

Colors selectors in the HSV space (Hue Saturation Value).

The color can be adjusted with the color wheel or individual channel sliders.

### HSL

![HSL View](/img/screenshots/colors/hsl.png)

Colors selectors in the HSL space (Hue Saturation Lightness).

The color can be adjusted with the color wheel or individual channel sliders.

### RGB

![RGB View](/img/screenshots/colors/rgb.png)

Colors selectors in the RGB space (Red Green Blue).

The color can be adjusted with the individual channel sliders.

### CMYK

![CMYK View](/img/screenshots/colors/cmyk.png)

Colors selectors in the CMYK space (Cyan Magenta Yellow Black).

The color can be adjusted with the individual channel sliders.

### Palette

![Palette View](/img/screenshots/colors/palette.png)

Here you can select, create, modify color palettes.

Opening and saving supports Gimp Palette files (.gpl).

Clicking on one of the colors, selects it as the current color.

Stroke
------

This view shows various stroke (outline) settings.

### Stroke Style

![Stroke Style Dock](/img/screenshots/stroke/style.png)

At the bottom it shows a preview of the current settings.

The "Width" spin box determines how thick the stroke is.

The "Cap" buttons determine the style of the ends of the line.

The "Join" buttons determine the style of sharp corners.

The spin box after the "Join" buttons determines how far a Miter join can reach.

### Stroke Color

![Stroke Style Dock](/img/screenshots/stroke/color.png)

This tab selects the stroke color in a similar way as to how the [Fill View](#Fill)
selects the fill color.


Layers
------

![Stroke Style Dock](/img/screenshots/views/layers.png)

This view shows layers and shapes.

From here you can rename them by double clicking on their name.
You can hide and lock them by clicking on the eye and padlock icons respectively.

You can also select a grouping color by clicking on the rectangle to thee left.
This has no effect other than changing how layers are shown in this view to help organize them.

### Context Menu

![Context Menu](/img/screenshots/tools/shape_menu.png)

Right clicking on an item will bring a context menu with quick actions for that item.


Timeline
--------

![Timeline Dock](/img/screenshots/views/timeline/timeline.png)

The timeline view allows to manage keyframes for the animatable properties of the active object.

### Playback Buttons

![Playback Buttons](/img/screenshots/views/timeline/buttons.png)

At the top of the Timeline view shows the current frame, and various playback buttons.

The frame spin box shows the current frame, and allows jumping to a specific frame by typing its number.

<img src="/img/ui/icons/media-playback-start.svg" width="32"> Starts and pauses playback<br/>
<img src="/img/ui/icons/media-playlist-repeat.svg" width="32"> Toggles whether playback loops the animation<br/>
<img src="/img/ui/icons/go-first.svg" width="32"> Jumps to the first frame<br/>
<img src="/img/ui/icons/go-previous.svg" width="32"> Jumps to the previous frame<br/>
<img src="/img/ui/icons/go-next.svg" width="32"> Jumps to the next frame<br/>
<img src="/img/ui/icons/go-last.svg" width="32"> Jumps to the last frame<br/>
<img src="/img/ui/icons/media-record.svg" width="32"> Record keyframes. When enabled, all changes made on the canvas are added as keyframes.<br/>

### Property List

![Property List](/img/screenshots/views/timeline/property_list.png)

On its left handside, the the Timeline view has the list of properties: showing their name, current value, and animation status.

The animation statuses are as follow:

<img alt="Green Circle" src="/img/ui/keyframe_status/not-animated.svg" width="32" /> Not animated<br/>
<img alt="Yellow Key" src="/img/ui/keyframe_status/key.svg" width="32" /> Currently on a keyframe<br/>
<img alt="Multiple Circles" src="/img/ui/keyframe_status/tween.svg" width="32" /> "Tweening" value, between keyframes<br/>
<img alt="Red Triangle" src="/img/ui/keyframe_status/mismatch.svg" width="32" /> Animated value that has been moved without adding a keyframe

### Keyframe Timeline

The right handside of the Timeline view shows the actual timeline.


Pressing left/right arrow keys changes the current frame when this area is focused.


#### Time Bar

![Time Bar](/img/screenshots/views/timeline/time_bar.png)

The bar at the top highlights the current frame.
Clicking or dragging with the mouse will jump to the frame whose number is under the mouse cursor.

#### Keyframe Area

![Keyframe Area](/img/screenshots/views/timeline/keyframe_area.png)

Under that, there are rows for each of the animatable properties, and icons showing the keyframes for said properties.

The keyframes icons can be selected by clicking (To select multiple Shift-click or Ctrl-click). and dragged with the mouse to change their time.

The keyframe icons show different shapes and colors depending on the easing transition of the keyframe before and after the icon.

<img alt="Yellow Diamond" src="/img/ui/keyframe/linear.svg" width="32" /> Linear, values change from the start to the end of the keyframe at a constant rate<br/>
<img alt="Blue Circle" src="/img/ui/keyframe/ease.svg" width="32" /> Smooth Easing, values change more slowly at the start or end of the keyframe<br/>
<img alt="Red Square" src="/img/ui/keyframe/hold.svg" width="32" /> Hold, the value remains constant until the next keyframe<br/>
<img alt="Purple Circle" src="/img/ui/keyframe/custom.svg" width="32" /> Custom curve, defined with the dialog described in [Custom Easing Dialog](#custom-easing-dialog)<br/>

*Ctrl + Mouse Wheel* will zoom the area with respect to time, *Mouse Wheel* by itself moves left or right.
To scroll up and down either use the scrollbar to the side or use the *Mouse Wheel* on the property list.

#### Context Menus

Right clicking on the row of a property, will show a context menu with the name of the property and the ability to add a keyframe:

![Property Menu](/img/screenshots/views/timeline/property_menu.png)

Clicking on "Add Keyframe" will add a keyframe at the current frame with the current value of the property.

Right clicking on an existing keyframe icon, will show a menu with options relating to that keyframe:

![Keyframe Menu](/img/screenshots/views/timeline/keyframe_menu.png)

Clicking on "Custom..." for the incoming or outgoing transitions will show the [Custom Easing Dialog](#custom-easing-dialog).

### Custom Easing Dialog

![Custom Easing Dialog](/img/screenshots/views/timeline/custom_easing.png)

This dialog gives full freedom to specify easing curves.

The curve represents how the value changes between two keyframes:
the horizontal axis represents the time between the two keyframes
and the vertical axis is the interpolation factor between the two values.

To choose a preset transition for one of the two ends, you can select a value from the dropdowns.


Properties
----------

![Property View](/img/screenshots/views/properties.png)

The Properties view allows the user to view or modify any property of the active object.

To edit a property double click on its value.

Animatable properties are highlighted as explained in [Property List](#property-list).


Undo History
------------

![Undo History View](/img/screenshots/views/undo.png)

This view allows you to see the action history for the current document,
clicking on the listed actions will revert the document to that action.

Useful for when you want to undo or redo several actions at once.

A little icon will be shown next to the action corresponding to the last time you saved the document.


Gradients
---------

![Gradients View](/img/screenshots/views/gradients/gradients_view.png)

This view shows the gradients used in the document.

It's hidden by default, to show it use the *View* > *Views* > *Gradients* menu action.

At the top of the view is the list of gradients, showing a preview, its name,
and the number of shapes using it.

Below that, you have a few buttons:

* <img src="/img/ui/icons/list-add.svg" width="32" /> Adds a new gradient based on the current colors.
* <img src="/img/ui/icons/folder.svg" width="32" /> Adds a new gradients from a list of presets.
* <img src="/img/ui/icons/list-remove.svg" width="32" /> Removes the selected gradient.

Then buttons to apply the selected griandient for fill or stroke:

<img src="/img/ui/icons/paint-gradient-linear.svg" width="32" /> for a linear gradient.<br/>
<img src="/img/ui/icons/paint-gradient-radial.svg" width="32" /> for a radial gradient.

Click on the checked button to remove the gradient from an object.

### Presets

![Gradients Presets](/img/screenshots/views/gradients/presets.png)

This menu shows a drop down with the available presets, based on <https://webgradients.com/>.

### Editing from the List

You can rename gradients by double-clicking on their name and typing a new name.

Similarly, you can double click on the preview to make it editable:

![Gradients View](/img/screenshots/views/gradients/gradients_view.png)

While in this mode, you can drag the lines representing the gradient stops to move them,
drag them off the rectangle to remove them, and double click to show a dialog to set the color.

Right-clicking shows this menu:

![Gradients View](/img/screenshots/views/gradients/edit_context_menu.png)


### Editing from the Canvas

With the [edit tool](tools.md#edit-tool) active, the gradient controls will appear on the canvas.<br/>

Dragging the end points will change the extent of the gradient and dragging the smaller handles
will change the offset of the corresponding gradient stop.

For a Linear gradient:

![Linear Gradient](/img/screenshots/views/gradients/linear.png)

For a radial gradient:

![Radial Gradient](/img/screenshots/views/gradients/radial.png)

When you have a radial gradient, you can shift click on the start handle to reveal
a new X-shaped handle that controls the highlight position for the gradient:

![Radial Highlight](/img/screenshots/views/gradients/radial_highlight.png)

To hide it again, shift-click on it.


Swatch
------

![Swatch View](/img/screenshots/views/swatch/swatch.png)

This view shows the colors in the document swatch.

The document swatch is a palette specific to the open document.
When an object is assigned a color from the document swatch, it gets linked to it.
Modifying the color in the swatch will reflect the change in all linked objects,
so you can recolor multiple objects at once.

The view shows the palette for the document swatch, which each color in its own
rectangle. There is an extra rectangle that's used to unlink the selected object
from the swatch (the object will retain its color but modifying the swatch will no
longer affect that object).

These are the buttons at the bottom:


<img src="/img/ui/icons/open-menu-symbolic.svg" width="32" /><br/>
Clicking and holding on this button will show [a menu with extra options](#swatch-extra-options).

<img src="/img/ui/icons/list-add.svg" width="32" /><br/> Adds the current fill color to the swatch.
  If an object is selected, its fill color will be linked to the swatch.

<img src="/img/ui/icons/list-remove.svg" width="32" /><br/> Removes the last clicked color from the swatch

### Swatch Extra Options

![Menu Extra](/img/screenshots/views/swatch/extra_menu.png)

<img src="/img/ui/icons/document-export.svg" width="32" /> *Generate* <br/>
Pulls the colors off the open document and link all objects to the swatch

<img src="/img/ui/icons/document-open.svg" width="32" /> *From Palette...*<br/>
Shows a [dialog](#from-palette) with option to populate the swatch from an existing palette.

<img src="/img/ui/icons/document-save.svg" width="32" /> *Save Palette*<br/>
Saves the swatch and it will show up as a [palette in the Fill and Stroke views](#palette).

### Mouse interactions

Left-clicking on one of the rectangles of the swatch will link the fill color of the selected object
(or unlink if you click the rectangle marked with an X).

Holding Shift when you click will affect the stroke color instead of the fill color.

Right clicking will show the following context menu:

![Context Menu](/img/screenshots/views/swatch/context_menu.png)

At the top of the menu you see the name of the color

<img src="/img/ui/icons/edit-rename.svg" width="32" /> *Rename...* <br/>
Shows a dialog where you can change the name of the color.

<img src="/img/ui/icons/color-management.svg" width="32" /> *Edit Color...* <br/>
Shows a dialog where you can change the color

<img src="/img/ui/icons/list-remove.svg" width="32" /> *Remove* <br/>
Will remove the color from the swatch.
This will have no visual changes, but all objects previously linked to this
color will be unlinked.

<img src="/img/ui/icons/edit-duplicate.svg" width="32" /> *Duplicate* <br/>
Will add a new identical color to the swatch.

<img src="/img/ui/icons/list-remove.svg" width="32" /> *Set as fill* <br/>
Does the same as clicking

<img src="/img/ui/icons/format-fill-color.svg" width="32" /> *Set as stroke* <br/>
Does the same as shift-clicking

<img src="/img/ui/icons/format-stroke-color.svg" width="32" /> *Link shapes with matching colors* <br/>
Searches the document for all shapes with the same color as the one in the rectangle you clicked, and links them to the swatch.

### From Palette

![From Palette Dialog](/img/screenshots/views/swatch/from_palette.png)

At the top it shows a dropdown with the palettes currently loaded by glaxnimate.
The selected item will define the colors used by the document swatch.

To add more palettes, you can do so in the [Palette section of the fill View](#palette).

* *Overwrite on save* means that clicking *Save Palette* for the document swatch
  will overwrite the selected palette.
* *Link shapes matching colors* When checked, Glaxnimate will scan the document
  for objects with colors in the palette and link them to the swatch.
* *Remove existing colors* If checked, any existing colors in the document swatch
  will be removed before adding new ones from the palette.
  Note that this means all objects will become unlinked.


Script Console
--------------

![Script Console](/img/screenshots/views/console.png)

This view (hidden by default) allows you to inspect and modify the open document
with Python.

See [Scripting](/contributing/scripting/index.md) for details.


Logs
----

![Log View](/img/screenshots/views/log.png)

This view (hidden by default) shows internal reporting, errors, etc.
