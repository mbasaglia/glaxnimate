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
|`assets`   |`array` |A list of [assets](#asset)
|`layers`   |`array` |A list of [layers](#layer) (See: [Lists of layers and shapes](#lists-of-layers-and-shapes))
|`v`        |`string`|Lottie version, on very old versions some things might be different from what is explained here|

Also has attributes from [Visual Object](#visual-object).

## Layer

There are several layer types, which is specified by the `ty` attribute:

| `ty` | Layer Type | Description |
|------|------------|-------------|
|`0`|[Precomposition](#precomplayer)|Renders a [Precomposition](#precomposition)|
|`1`|[Solid Color](#solidcolorlayer)|Static rectangle filling the canvas with a single color|
|`2`|[Image](#imagelayer)|Renders an [Image](#image)|
|`3`|[Null (Empty)](#nulllayer)|No contents, only used for parenting|
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

### ShapeLayer

### PrecompLayer

### NullLayer

### TextLayer

### SolidColorLayer


## Shape

TODO

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

{lottie:../../examples/layer_order.json:512:512:}

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
|`h`|[0-1 `int`](#booleans)|Whether it's a hold frame|

If `h` is present and it's 1, you don't need `i` and `o`, as the property will keep the same value
until the next keyframe.


#### Easing Handles

They are objects with `x` and `y` attributes, which are numbers within 0 and 1.

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
