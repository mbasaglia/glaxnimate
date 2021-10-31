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
| `0`   | Corner        | The two tangents are independent                                          |
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
