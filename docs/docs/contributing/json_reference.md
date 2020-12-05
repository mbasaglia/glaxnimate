Authors: Mattia Basaglia

# Glaxnimate file format

Glaxnimate files are stored as JSON.

The top-level object represents the document, it has the following fields:

| name          | type                                  | required  | description               |
| ---           | ---                                   | ---       | ---                       |
| `format`      | [Format Metadata](#format-metadata)   | yes       | Describes the format used |
| `metadata`    | `object`                              | no        | Can contain anything      |
| `defs`        | [Defs](#defs)                         | no        | Defines assets            |
| `animation`   | [MainComposition](#maincomposition)   | yes       | Animation object          |


## Basic Types

### Format Metadata

Object describing the file format.

| name                  | type     | required | description                               |
| --------------------- | -------- | --- | ---------------------------------------------- |
| `generator`           | `string` | no  | Program used to create the file                |
| `generator_version`   | `string` | no  | Version of the program used to create the file |
| `format_version`      | `int`    | yes | Version of the format specs, currently `2`     |

### Point

Represents a point in 2D space

| name  | type      | required  | description   |
| ----- | --------- | --------- | ------------- |
| `x`   | `float`   | yes       | X coordinate  |
| `y`   | `float`   | yes       | Y coordinate  |


### Size

Represents a size

| name      | type      | required  | description   |
| -----     | --------- | --------- | ------------- |
| `width`   | `float`   | yes       |               |
| `height`  | `float`   | yes       |               |


### Bezier

A polybezier.

| name      | type                                      | required  | description                   |
| -----     | ---------                                 | --------- | -------------                 |
| `closed`  | `bool`                                    | no        | Whether the bezier is closed  |
| `points`  | array of [Bezier Points](#bezier-point)   | yes       |                               |

### Bezier Point

| name      | type              | required  | description                   |
| --------- | ----------------- | --------- | -------------                 |
| `pos`     | [Point](#point)   | yes       | Vertex Position               |
| `tan_in`  | [Point](#point)   | yes       | Incoming tangent (absolute)   |
| `tan_out` | [Point](#point)   | yes       | Outgoing tangent (absolute)   |
| `type`    | `int`             | yes       | See below for possible values |

Point Types:

| value | name          | description                                                               |
| ----- | ------------- | ------------------------------------------------------------------------- |
| `0`   | Corner        | The two tangents are indipendent                                          |
| `1`   | Smooth        | The two tangents are on the same line but their length can be different   |
| `2`   | Symmetrical   | The two tangents are on the same line and have the same length            |


### Gradient Stop


| name      | type              | required  | description                                           |
| --------- | ----------------- | --------- | -------------                                         |
| `offset`  | `float`           | yes       | Value in [0, 1] determining the offset of this stop   |
| `color`   | [Color](#color)   | yes       | Color of the stop                                     |


### Color

A string representing color values, it starts with `#` and is followed by
2 hexadecimal digits per color component (Red, Green, Blue).

With an optional 2 extra hexadecimal digits to represent transparency.


### UUID

String representing a unique identifier, in the form

    {00000000-0000-0000-0000-000000000000}

## Object Types

Most objects fall in this category, their type is represented by the `__type__` attribute.

Each type inherits properties from its parent type.

If a specific property requires a given `__type__`, you can use one of its sub-types.
### Asset

Base types:

* [ReferenceTarget](#referencetarget)

Sub types:

* [BrushStyle](#brushstyle)
* [GradientColors](#gradientcolors)
* [Bitmap](#bitmap)

### BrushStyle

Base types:

* [Asset](#asset)

Sub types:

* [NamedColor](#namedcolor)
* [Gradient](#gradient)

### NamedColor

Base types:

* [BrushStyle](#brushstyle)

Properties:

| name    | type            | docs | 
| ------- | --------------- | ---- | 
| `color` | [Color](#color) |      | 

### GradientColors

Base types:

* [Asset](#asset)

Properties:

| name     | type                                     | docs | 
| -------- | ---------------------------------------- | ---- | 
| `colors` | array of [Gradient Stop](#gradient-stop) |      | 

### Gradient

Base types:

* [BrushStyle](#brushstyle)

Properties:

| name          | type                          | docs                                           | 
| ------------- | ----------------------------- | ---------------------------------------------- | 
| `colors`      | [UUID](#uuid)                 | References [GradientColors](#gradientcolors).  | 
| `type`        | [GradientType](#gradienttype) |                                                | 
| `start_point` | [Point](#point)               |                                                | 
| `end_point`   | [Point](#point)               |                                                | 
| `highlight`   | [Point](#point)               |                                                | 

### Bitmap

Base types:

* [Asset](#asset)

Properties:

| name       | type          | docs | 
| ---------- | ------------- | ---- | 
| `data`     | Base64 string |      | 
| `filename` | `string`      |      | 
| `format`   | `string`      |      | 
| `width`    | `int`         |      | 
| `height`   | `int`         |      | 

### Precomposition

Base types:

* [Composition](#composition)

### Defs

Base types:

* [Object](#object)

Properties:

| name              | type                                       | docs | 
| ----------------- | ------------------------------------------ | ---- | 
| `colors`          | array of [NamedColor](#namedcolor)         |      | 
| `images`          | array of [Bitmap](#bitmap)                 |      | 
| `gradient_colors` | array of [GradientColors](#gradientcolors) |      | 
| `gradients`       | array of [Gradient](#gradient)             |      | 
| `precompositions` | array of [Precomposition](#precomposition) |      | 

### ShapeElement

Base types:

* [DocumentNode](#documentnode)

Sub types:

* [Shape](#shape)
* [Modifier](#modifier)
* [Styler](#styler)
* [Group](#group)
* [PreCompLayer](#precomplayer)
* [Image](#image)

### Shape

Base types:

* [ShapeElement](#shapeelement)

Sub types:

* [Rect](#rect)
* [Ellipse](#ellipse)
* [PolyStar](#polystar)
* [Path](#path)

### Modifier

Base types:

* [ShapeElement](#shapeelement)

### Styler

Base types:

* [ShapeElement](#shapeelement)

Sub types:

* [Fill](#fill)
* [Stroke](#stroke)

Properties:

| name      | type            | docs                                   | 
| --------- | --------------- | -------------------------------------- | 
| `color`   | [Color](#color) |                                        | 
| `opacity` | `float`         |                                        | 
| `use`     | [UUID](#uuid)   | References [BrushStyle](#brushstyle).  | 

### Rect

Base types:

* [Shape](#shape)

Properties:

| name       | type            | docs | 
| ---------- | --------------- | ---- | 
| `position` | [Point](#point) |      | 
| `size`     | [Size](#size)   |      | 
| `rounded`  | `float`         |      | 

### Ellipse

Base types:

* [Shape](#shape)

Properties:

| name       | type            | docs | 
| ---------- | --------------- | ---- | 
| `position` | [Point](#point) |      | 
| `size`     | [Size](#size)   |      | 

### PolyStar

Base types:

* [Shape](#shape)

Properties:

| name           | type                  | docs | 
| -------------- | --------------------- | ---- | 
| `type`         | [StarType](#startype) |      | 
| `position`     | [Point](#point)       |      | 
| `outer_radius` | `float`               |      | 
| `inner_radius` | `float`               |      | 
| `angle`        | `float`               |      | 
| `points`       | `int`                 |      | 

### Path

Base types:

* [Shape](#shape)

Properties:

| name     | type              | docs | 
| -------- | ----------------- | ---- | 
| `shape`  | [Bezier](#bezier) |      | 
| `closed` | `bool`            |      | 

### Group

Base types:

* [ShapeElement](#shapeelement)

Sub types:

* [Layer](#layer)

Properties:

| name      | type                                   | docs | 
| --------- | -------------------------------------- | ---- | 
| `shapes`  | array of [ShapeElement](#shapeelement) |      | 
| `opacity` | `float`                                |      | 

### Layer

Base types:

* [Group](#group)

Properties:

| name     | type          | docs                         | 
| -------- | ------------- | ---------------------------- | 
| `parent` | [UUID](#uuid) | References [Layer](#layer).  | 
| `render` | `bool`        |                              | 

### PreCompLayer

Base types:

* [ShapeElement](#shapeelement)

Properties:

| name          | type          | docs                                           | 
| ------------- | ------------- | ---------------------------------------------- | 
| `composition` | [UUID](#uuid) | References [Precomposition](#precomposition).  | 
| `size`        | [Size](#size) |                                                | 
| `opacity`     | `float`       |                                                | 

### Fill

Base types:

* [Styler](#styler)

Properties:

| name        | type          | docs | 
| ----------- | ------------- | ---- | 
| `fill_rule` | [Rule](#rule) |      | 

### Stroke

Base types:

* [Styler](#styler)

Properties:

| name          | type          | docs | 
| ------------- | ------------- | ---- | 
| `width`       | `float`       |      | 
| `cap`         | [Cap](#cap)   |      | 
| `join`        | [Join](#join) |      | 
| `miter_limit` | `float`       |      | 

### Image

Base types:

* [ShapeElement](#shapeelement)

Properties:

| name    | type          | docs                           | 
| ------- | ------------- | ------------------------------ | 
| `image` | [UUID](#uuid) | References [Bitmap](#bitmap).  | 

### Object

Sub types:

* [ReferenceTarget](#referencetarget)
* [AnimationContainer](#animationcontainer)
* [StretchableTime](#stretchabletime)
* [Transform](#transform)
* [Defs](#defs)

### ReferenceTarget

Base types:

* [Object](#object)

Sub types:

* [DocumentNode](#documentnode)
* [Asset](#asset)

Properties:

| name   | type          | docs | 
| ------ | ------------- | ---- | 
| `uuid` | [UUID](#uuid) |      | 
| `name` | `string`      |      | 

### DocumentNode

Base types:

* [ReferenceTarget](#referencetarget)

Sub types:

* [Composition](#composition)
* [ShapeElement](#shapeelement)

Properties:

| name          | type            | docs | 
| ------------- | --------------- | ---- | 
| `group_color` | [Color](#color) |      | 
| `visible`     | `bool`          |      | 
| `locked`      | `bool`          |      | 

### AnimationContainer

Base types:

* [Object](#object)

Properties:

| name          | type    | docs | 
| ------------- | ------- | ---- | 
| `first_frame` | `float` |      | 
| `last_frame`  | `float` |      | 

### StretchableTime

Base types:

* [Object](#object)

Properties:

| name         | type    | docs | 
| ------------ | ------- | ---- | 
| `start_time` | `float` |      | 
| `stretch`    | `float` |      | 

### Transform

Base types:

* [Object](#object)

Properties:

| name           | type                  | docs | 
| -------------- | --------------------- | ---- | 
| `anchor_point` | [Point](#point)       |      | 
| `position`     | [Point](#point)       |      | 
| `scale`        | [Vector2D](#vector2d) |      | 
| `rotation`     | `float`               |      | 

### Composition

Base types:

* [DocumentNode](#documentnode)

Sub types:

* [MainComposition](#maincomposition)
* [Precomposition](#precomposition)

Properties:

| name     | type                                   | docs | 
| -------- | -------------------------------------- | ---- | 
| `shapes` | array of [ShapeElement](#shapeelement) |      | 

### MainComposition

Base types:

* [Composition](#composition)

Properties:

| name     | type    | docs | 
| -------- | ------- | ---- | 
| `fps`    | `float` |      | 
| `width`  | `int`   |      | 
| `height` | `int`   |      | 

## Enumerations

### PointType

| value         | docs | 
| ------------- | ---- | 
| `Corner`      |      | 
| `Smooth`      |      | 
| `Symmetrical` |      | 

### GradientType

| value    | docs | 
| -------- | ---- | 
| `Linear` |      | 
| `Radial` |      | 

### StarType

| value     | docs | 
| --------- | ---- | 
| `Star`    |      | 
| `Polygon` |      | 

### Rule

| value     | docs | 
| --------- | ---- | 
| `NonZero` |      | 
| `EvenOdd` |      | 

### Cap

| value       | docs | 
| ----------- | ---- | 
| `ButtCap`   |      | 
| `RoundCap`  |      | 
| `SquareCap` |      | 

### Join

| value       | docs | 
| ----------- | ---- | 
| `MiterJoin` |      | 
| `RoundJoin` |      | 
| `BevelJoin` |      | 

### Descriptive

| value    | docs | 
| -------- | ---- | 
| `Hold`   |      | 
| `Linear` |      | 
| `Ease`   |      | 
| `Custom` |      | 

