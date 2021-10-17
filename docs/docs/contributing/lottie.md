Authors: Mattia Basaglia

# A human's guide to the Lottie format

Lottie is a vector animation format, using JSON to represent its data.

This guide aims to provide a human-readable description of the format and how
everything works within it.

Lottie has several implementations and some things might vary from player to player,
this guide tries to follow the behaviour of [lottie web](https://github.com/airbnb/lottie-web/)
which is the reference implementation.

If you want more details on the JSON attributes you can also check my
[machine-generated documentation](https://mattbas.gitlab.io/python-lottie/group__Lottie.html#details).

Note that some lottie players require certain JSON keys to be presents before others in the file
to play properly.

# Structure

## Animation

This is the top-level JSON object, describing the document, layers, assets, etc.

The most notable attributes are:

|Attribute|Type|Description|
|---------|----|-----------|
|`w`, `h`   |`number`|Width and height of the animation|
|`fr`       |`number`|Framerate (frames per second)|
|`ip`       |`number`|"In Point", which frame the animation starts at (usually 0)|
|`op`       |`number`|"Out Point", which frame the animation stops/loops at, which makes it the duration in frames|
|`assets`   |`array` |An array of [assets](#asset)
|`layers`   |`array` |An array of [layers](#layer) (See: [Lists of layers and shapes](#lists-of-layers-and-shapes))
|`v`        |`string`|Lottie version, on very old versions some things might be different from what is explained here|
|`ddd`|[0-1 `int`](#booleans)|Whether the animation has 3D layers. Lottie doesn't actually support 3D stuff so this should always be 0|

Also has attributes from [Visual Object](#visual-object).

## Layer

There are several layer types, which is specified by the `ty` attribute:

| `ty` | Layer Type | Description |
|------|------------|-------------|
|`0`|[Precomposition](#precomplayer)|Renders a [Precomposition](#precomposition)|
|`1`|[Solid Color](#solidcolorlayer)|Static rectangle filling the canvas with a single color|
|`2`|[Image](#imagelayer)|Renders an [Image](#image)|
|`3`|[Null (Empty)](#nulllayer)|No contents, only used for [parenting](#parenting)|
|`4`|[Shape](#shapelayer)|Contains a [list](#lists-of-layers-and-shapes) of [shapes](#shape)|
|`5`|[Text](#textlayer)|Renders text|
|`6`|Audio||
|`7`|Video Placeholder||
|`8`|Image Sequence||
|`9`|Video||
|`10`|Image Placeholder||
|`11`|Guide||
|`12`|Adjustment||
|`13`|Camera||
|`14`|Light||

Each layer type has its own properties but there are several common properties:

They also have attributes from [Visual Object](#visual-object).

|Attribute|Type|Name|Description|
|---------|----|----|-----------|
|`ddd`  |[0-1 `int`](#booleans) |Threedimensional   |Whether the layer is 3D. Lottie doesn't actually support 3D stuff so this should always be 0||
|`hd`   |`boolean`              |Hidden             |Whether the layer is hidden|
|`ty`   |`integer`              |Type               |One of the values as seen before|
|`ind`  |`integer`              |Index              |Layer index for [parenting](#parenting)|
|`parent`|`integer`             |Parent             |Parent index for [parenting](#parenting)|
|`sr`   |`number`               |Time stretch       | |
|`ks`   |[Transform](#transform)|Transform          |Layer transform|
|`ao`   |[0-1 `int`](#booleans) |Auto Orient        |[Auto orient](#auto-orient)|
|`ip`   |`number`               |In Point           |Frame when the layers becomes visible|
|`op`   |`number`               |Out Point          |Frame when the layers becomes invisible|
|`st`   |`number`               |Start time         ||
|`bm`   |`integer`              |[Blen Mode](https://mattbas.gitlab.io/python-lottie/group__Lottie.html#lottie_BlendMode)||
|`tt`   |`integer`              |[Matte Mode](https://mattbas.gitlab.io/python-lottie/group__Lottie.html#lottie_MatteMode)|See [mattes](#mattes)|
|`td`   |`integer`              |Matte Target       |See [mattes](#mattes)|
|`hasMask`|`boolean`            |Has Mask           |Whether the layer has masks applied|
|`masksProperties`|`array`      |Masks              |[Masks](#masks) for the layer|
|`ef`   |`array`                |Effects            |[Effects](#effect) for the layer|

The layer is only visible between its `ip` and `op`.
If they match the corresponding attributes in [Animation](#animation), the layer
will be visible the whole time, otherwise it will become visible at the frame `ip`
and disappear at `op`.


### Parenting

Within a list of layers, the `ind` attribute (if present) must be unique.

Layers having a `parent` attribute matching another layer will inherit their
parent's transform (except for opacity).

Basically you need multiply the transform matrix by the parent's transform matrix
to get a child layer's final transform.

### Auto Orient

When true, if the transform position is animated, it rotates the layer along the
path the position follows.

In the following example there are two arrows animated along the same path,
with no rotation when the position moves towards the right.

The transparent one keeps its rotation the same (`ao` is 0), while the solid one
follows the path (`ao` is 1).

{lottie:../../examples/auto_orient.json:512:512:-}

### Mattes

A matte allows using a layer as a mask for another layer.

The way it works is the layer defining the mask has a `tt` attribute with the
appropriate [value](https://mattbas.gitlab.io/python-lottie/group__Lottie.html#lottie_MatteMode)
and it affects the layer on top (the layer before it in the layer list).

In this example there's a layer with a rectangle and a star being masked by an ellipse:
{lottie:../../examples/matte.json:512:512:-}


## ShapeLayer

Renders vector data.

The only special property for this layer is **shapes**, an [array](#lists-of-layers-and-shapes) of [shapes](#shape).

## PreCompLayer

This layer renders a [precomposition](#precomposition).

|Attribute|Type|Name|Description|
|-----------|----|----|-----------|
|`refId`    |`string` |Reference ID|ID of the precomp as specified in the assets|
|`w`        |`number` |Width||
|`h`        |`number` |Height||
|`tm`       |Animated `number`|Time Remapping||

### Time remapping

The `tm` property maps the time in seconds of the precomposition to show.

Basically you get the value of `tm` at the current frame, then assume that's
a time in seconds since the start of the animation, and render the corresponding
frame of the precomposition.

Follows an example of this, here there are two layers showing the same
precomposition, the one at the top right keeps the original time while the bottom
one remaps time as follows:

* frame 0 (0s) maps to 0s (frame 0) in the precomp
* frame 30 (0.5s) maps to 3s (frame 180) in the precomp
* frame 60 (1s) maps to 1.5s (frame 90) in the precomp
* frame 180 (3s) maps to 3s (frame 180) in the precomp

Basically it makes the precomp play in the first half second, then rewind
to half way for the next half second, and plays back to the end for the remaining
2 seconds.

{lottie:../../examples/remapping.json:512:512:-}


## NullLayer

This layer doesn't have any special properties.

It's often used by animators as a parent to multiple other layers.

## TextLayer

TODO

## ImageLayer

This layer renders a static [image](#image).


|Attribute|Type|Name|Description|
|-----------|----|----|-----------|
|`refId`    |`string` |Reference ID|ID of the image as specified in the assets|

## SolidColorLayer

This layer represents a rectangle with a single color.

Anything you can do with solid layers, you can do better with a shape layer
and a rectangle shape since none of this layer's own properties can be animated.

|Attribute|Type|Name|Description|
|-----------|----|----|-----------|
|`sc`       |`string` |Solid Color|Color of the layer, unlike most other places, the color is a `#rrggbb` hex string |
|`sw`       |`number` |Solid Width||
|`sh`       |`number` |Solid Height||


## Shape

Lottie considers everything related to vector data as a "shape" but I think
it's worth distinguishing across a few categories:

* **Actual Shapes** These provide only the shape information, but no styling
* **Style** These provide styling info (like fill and stroke)
* **Group** This is a shape that contains other shape
* **Modifier** These change other shapes
* **Transform** Special shape that defines the transforms in a group shape

All shapes have the attributes from [Visual Object](#visual-object) and the following:



## Actual Shapes

### Rectangle

### Ellipse

### PolyStar

### Path

## Style

### Fill

### Stroke

### Gradient Fill / Stroke

## Group

### Transform Shape

## Modifiers

### Repeater

### Trim

### Rounded Corners

### Pucker / Bloat

## Asset

TODO

### Image

### Precomposition

## Booleans

In some places boolean values are shown as booleans in the JSON (`true`/`false`).
In other places they are shown as integers with `0` or `1` as values.

## Colors

Colors are represented as arrays with values between 0 and 1 for the RGB components.

for example:

* {lottie_color:1, 0, 0}
* {lottie_color:1, 0.5, 0}

Note sometimes you might find color values with 4 components (the 4th being alpha)
but most player ignore the last component.

## Gradients

Gradients are represented as a flat array, showing offsets and RGB components.

There are two possible representations, with alpha, and without.

### Gradients without transparency

The array is a sequence of `offset`, `red`, `green`, `blue` components for each
color. all values are between 0 and 1

So let's say you want these colors:

* {lottie_color_255:41, 47, 117}
* {lottie_color_255:50, 80, 176}
* {lottie_color_255:196, 217, 245}

the array will look like the following:

`[0, 0.161, 0.184, 0.459, 0.5, 0.196, 0.314, 0.69, 1, 0.769, 0.851, 0.961]`

| Value     | Description |
|-----------|---|
| `0`       | Offset of the 1st color (`0` means at the start) |
| `0.161`   | Red component for the 1st color |
| `0.184`   | Green component for the 1st color |
| `0.459`   | Blue component for the 1st color |
| `0.5`     | Offset of the 2nd color (`0.5` means half way) |
| `0.196`   | Red component for the 2nd color |
| `0.314`   | Green component for the 2nd color |
| `0.69`    | Blue component for the 2nd color |
| `1`       | Offset of the 3rd color (`1` means at the end) |
| `0.769`   | Red component for the 3rd color |
| `0.851`   | Green component for the 3rd color |
| `0.961`   | Blue component for the 3rd color |

### Gradients with transparency

Alpha is added at the end, repeating offsets and followed by alpha for each colors

So assume the same colors as before, but opacity of 80% for the first color and 100% for the other two.

The array will look like this:

`[0, 0.161, 0.184, 0.459, 0.5, 0.196, 0.314, 0.69, 1, 0.769, 0.851, 0.961, 0, 0.8, 0.5, 1, 1, 1]`

It's the same array as the case without transparency but with the following values added at the end:


| Value     | Description |
|-----------|---|
| `0`       | Offset of the 1st color (`0` means at the start) |
| `0.8`     | Alpha component for the 1st color |
| `0.5`     | Offset of the 2nd color (`0.5` means half way) |
| `1`       | Alpha component for the 2nd color |
| `1`       | Offset of the 3rd color (`1` means at the end) |
| `1`       | Alpha component for the 3rd color |


## Lists of layers and shapes

Such lists appear Precomposition, Animation, ShapeLayer, and Grop.

In such lists, items coming first will be rendered on top

So if you have for example: `[Ellipse, Rectangle]`

The ellipse will show on top of the rectangle:

{lottie:../../examples/layer_order.json:512:512:-}

This means the render order goes from the last element to the first.


## Visual Object

Most visual objects share these attributes:


|Attribute|Type|Description|
|---------|----|-----------|
|`nm`   |`string`|Name, usually what you see in an editor for layer names, etc.|
|`mn`   |`string`|"Match Name", used in expressions|

## Animated Property

Animated properties have two attributes


|Attribute|Type|Description|
|---------|----|-----------|
|`a`|[0-1 `int`](#booleans)|Whether the property is animated. Note some old animations might not have this|
|`k`||Value or keyframes, this changes based on the value of `a`|

If `a` is `0`, then `k` just has the value of the property.

If `a` is `1`, `k` will be an array of keyframes.

### Keyframe


|Attribute|Type|Description|
|---------|----|-----------|
|`t`    |`number`|Keyframe time (in frames)|
|`s`    |Depends on the property|Value, note that sometimes properties for scalar values have the value is wrapped in an array|
|`i`,`o`|[Easing Handle](#easing-handles)|
|`h`    |[0-1 `int`](#booleans)|Whether it's a hold frame|

If `h` is present and it's 1, you don't need `i` and `o`, as the property will keep the same value
until the next keyframe.


#### Easing Handles

They are objects with `x` and `y` attributes, which are numbers within 0 and 1.
You might see these values wrapped around arrays.

They represent a cubic bezier, starting at `[0,0]` and ending at `[1,1]` where
the value determines the easing function.

The `x` axis represents time, a value of 0 is the time of the current keyframe,
a value of 1 is the time of the next keyframe.

The `y` axis represents the value interpolation factor, a value of 0
represents the value at the current keyframe, a value of 1 represents the
value at the next keyframe.

When you use easing you have two easing handles for the keyframe:

`o` is the "out" handle, and is the first one in the bezier, determines the curve
as it exits the current keyframe.


`i` is the "in" handle, and it's the second one in the bezier, determines the curve
as it enters the next keyframe.

#### Old Lottie Keyframes

Old lotties have an additional attribute for keyframes, `e` which works
similarly to `s` but represents the value at the end of the keyframe.

They also have a final keyframe with only the `t` attribute and you
need to determine its value based on the `s` value of the previous keyframe.

## Transform

This represents a layer or shape transform.

It has the properties from [Visual Object](#visual-object) and its own properties are all [animated](#animated-property):


|Attribute|Type|Name|Description|
|---------|----|----|-----------|
|`a`    |2D Vector|Anchor point |Position (relative to its parent) around which transformations are applied (ie: center for rotation / scale)|
|`p`    |2D Vector|Position     |Position / Translation|
|`s`    |2D Vector|Scale        |Scale factor, `100` for no scaling|
|`r`    |`number` |Rotation     |Rotation in degrees, clockwise|
|`sk`   |`number` |Skew         |Skew amount as an angle in degrees|
|`sa`   |`number` |Skew Axis    |Direction at which skew is applied, in degrees (`0` skews along the X axis, `90` along the Y axis)|

Sometimes `p` might be replaced by its individual components (`px` and `py`) animated independently.

### Transforming to a matrix

Assuming the matrix

{matrix}
a   c   0   0
b   d   0   0
0   0   1   0
tx  ty  0   1

Multiplications are right multiplications (`Next = Previous * StepOperation`).

If your transform is transposed (`tx`, `ty` are on the last column), perform left multiplication instead.

Perform the following operations on a matrix starting from the identity matrix (or the parent object's transform matrix):

Translate by `-a`:

{matrix}
1       0       0   0
0       1       0   0
0       0       1   0
-a[0]   -a[1]   0   1

Scale by `s/100`:

{matrix}
s[0]/100    0           0   0
0           s[1]/100    0   0
0           0           1   0
0           0           0   1


Rotate by `sa` (can be skipped if not skewing)

{matrix}
cos(sa)     sin(sa) 0 0
-sin(sa)    cos(sa) 0 0
0           0       1 0
0           0       0 1

Skew by `sk` (can be skipped if not skewing)

{matrix}
1   tan(sk) 0   0
0   1       0   0
0   0       1   0
0   0       0   1

Rotate by `-sa` (can be skipped if not skewing)

{matrix}
cos(-sa)   sin(-sa) 0 0
-sin(-sa)  cos(-sa) 0 0
0          0        1 0
0          0        0 1

Rotate by `-r`

{matrix}
cos(-r)    sin(-r)  0 0
-sin(-r)   cos(-r)  0 0
0          0        1 0
0          0        0 1

If you are handling an [auto orient](auto-orient) layer, evaluate and apply auto-orient rotation

Translate by `p`

{matrix}
1       0       0   0
0       1       0   0
0       0       1   0
p[0]    p[1]    0   1

## Mask

TODO

## Effect

TODO
