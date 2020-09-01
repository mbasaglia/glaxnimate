Authors: Mattia Basaglia

# Dockable Views

## Tools

## Colors

![Colors View](/img/screenshots/colors/main.png)

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

## Layers

## Timeline

![Timeline Dock](/img/screenshots/timeline/timeline.png)

The timeline view allows to manage keyframes for the animatable properties of the active object.

### Playback Buttons

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

![Property List](/img/screenshots/timeline/property_list.png)

On its left handside, the the Timeline view has the list of properties: showing their name, current value, and animation status.

The animation statuses are as follow:

<img alt="Green Circle" src="/img/ui/keyframe_status/not-animated.svg" width="32" /> Not animated<br/>
<img alt="Yellow Key" src="/img/ui/keyframe_status/key.svg" width="32" /> Currently on a keyframe<br/>
<img alt="Multiple Circles" src="/img/ui/keyframe_status/tween.svg" width="32" /> "Tweening" value, between keyframes<br/>
<img alt="Red Triangle" src="/img/ui/keyframe_status/mismatch.svg" width="32" /> Animated value that has been moved without adding a keyframe

### Keyframe Timeline

The right handside of the Timeline view shows the actual timeline.


#### Time Bar

![Time Bar](/img/screenshots/timeline/time_bar.png)

The bar at the top highlights the current frame.
Clicking or dragging with the mouse will jump to the frame whose number is under the mouse cursor.

#### Keyframe Area

![Keyframe Area](/img/screenshots/timeline/keyframe_area.png)

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

![Property Menu](/img/screenshots/timeline/property_menu.png)

Clicking on "Add Keyframe" will add a keyframe at the current frame with the current value of the property.

Right clicking on an existing keyframe icon, will show a menu with options relating to that keyframe:

![Keyframe Menu](/img/screenshots/timeline/keyframe_menu.png)

Clicking on "Custom..." for the incoming or outgoing transitions will show the [Custom Easing Dialog](#custom-easing-dialog).

### Custom Easing Dialog

![Custom Easing Dialog](/img/screenshots/timeline/custom_easing.png)

This dialog gives full freedom to specify easing curves.

The curve represents how the value changes between two keyframes:
the horizontal axis represents the time between the two keyframes
and the vertical axis is the interpolation factor between the two values.

To choose a preset transition for one of the two ends, you can select a value from the dropdowns.

## Properties

![Property View](/img/screenshots/property_view.png)

The Properties view allows the user to view or modify any property of the active object.

To edit a property double click on its value.

Animatable properties are highlighted as explained in [Property List](#property-list).


## Script Console

## Logs
