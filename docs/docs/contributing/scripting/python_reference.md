# glaxnimate

Constants:

| name          | type  | value                 | docs | 
| ------------- | ----- | --------------------- | ---- | 
| `__version__` | `str` | `'0.2.0-24-ga2110fe'` |      | 

# glaxnimate.utils

## glaxnimate.utils.Color

Properties:

| name    | type  | notes | docs                                     | 
| ------- | ----- | ----- | ---------------------------------------- | 
| `red`   | `int` |       | Red component between 0 and 255          | 
| `green` | `int` |       | Green component between 0 and 255        | 
| `blue`  | `int` |       | Blue component between 0 and 255         | 
| `alpha` | `int` |       | Transparency component between 0 and 255 | 
| `name`  | `str` |       |                                          | 

## glaxnimate.utils.Point

Properties:

| name     | type    | notes     | docs | 
| -------- | ------- | --------- | ---- | 
| `x`      | `float` |           |      | 
| `y`      | `float` |           |      | 
| `length` | `float` | Read only |      | 

## glaxnimate.utils.Size

Properties:

| name     | type    | notes | docs | 
| -------- | ------- | ----- | ---- | 
| `width`  | `float` |       |      | 
| `height` | `float` |       |      | 

<h3 id='glaxnimate.utils.Size.to_point'><a href='#glaxnimate.utils.Size.to_point'>to_point()</a></h3>

```python
to_point(self: glaxnimate.utils.Size) -> glaxnimate.utils.Point
```

## glaxnimate.utils.IntSize

Properties:

| name     | type  | notes | docs | 
| -------- | ----- | ----- | ---- | 
| `width`  | `int` |       |      | 
| `height` | `int` |       |      | 

<h3 id='glaxnimate.utils.IntSize.to_point'><a href='#glaxnimate.utils.IntSize.to_point'>to_point()</a></h3>

```python
to_point(self: glaxnimate.utils.IntSize) -> glaxnimate.utils.Point
```

## glaxnimate.utils.Vector2D

Properties:

| name | type    | notes | docs | 
| ---- | ------- | ----- | ---- | 
| `x`  | `float` |       |      | 
| `y`  | `float` |       |      | 

<h3 id='glaxnimate.utils.Vector2D.normalize'><a href='#glaxnimate.utils.Vector2D.normalize'>normalize()</a></h3>

```python
normalize(self: glaxnimate.utils.Vector2D) -> None
```

<h3 id='glaxnimate.utils.Vector2D.normalized'><a href='#glaxnimate.utils.Vector2D.normalized'>normalized()</a></h3>

```python
normalized(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Vector2D
```

<h3 id='glaxnimate.utils.Vector2D.to_point'><a href='#glaxnimate.utils.Vector2D.to_point'>to_point()</a></h3>

```python
to_point(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Point
```

<h3 id='glaxnimate.utils.Vector2D.length'><a href='#glaxnimate.utils.Vector2D.length'>length()</a></h3>

```python
length(self: glaxnimate.utils.Vector2D) -> float
```

<h3 id='glaxnimate.utils.Vector2D.length_squared'><a href='#glaxnimate.utils.Vector2D.length_squared'>length_squared()</a></h3>

```python
length_squared(self: glaxnimate.utils.Vector2D) -> float
```

## glaxnimate.utils.Rect

Properties:

| name           | type                           | notes | docs | 
| -------------- | ------------------------------ | ----- | ---- | 
| `left`         | `float`                        |       |      | 
| `right`        | `float`                        |       |      | 
| `top`          | `float`                        |       |      | 
| `bottom`       | `float`                        |       |      | 
| `center`       | [Point](#glaxnimateutilspoint) |       |      | 
| `top_left`     | [Point](#glaxnimateutilspoint) |       |      | 
| `top_right`    | [Point](#glaxnimateutilspoint) |       |      | 
| `bottom_right` | [Point](#glaxnimateutilspoint) |       |      | 
| `bottom_left`  | [Point](#glaxnimateutilspoint) |       |      | 
| `size`         | [Size](#glaxnimateutilssize)   |       |      | 

# glaxnimate.log

Logging utilities

<h2 id='glaxnimate.log.info'><a href='#glaxnimate.log.info'>info()</a></h2>

```python
info(arg0: str) -> None
```

<h2 id='glaxnimate.log.warning'><a href='#glaxnimate.log.warning'>warning()</a></h2>

```python
warning(arg0: str) -> None
```

<h2 id='glaxnimate.log.error'><a href='#glaxnimate.log.error'>error()</a></h2>

```python
error(arg0: str) -> None
```

# glaxnimate.io

Input/Output utilities

Constants:

| name       | type                                  | value | docs | 
| ---------- | ------------------------------------- | ----- | ---- | 
| `registry` | [IoRegistry](#glaxnimateioioregistry) |       |      | 

## glaxnimate.io.MimeSerializer

Properties:

| name         | type        | notes     | docs | 
| ------------ | ----------- | --------- | ---- | 
| `slug`       | `str`       | Read only |      | 
| `name`       | `str`       | Read only |      | 
| `mime_types` | `List[str]` | Read only |      | 

<h3 id='glaxnimate.io.MimeSerializer.serialize'><a href='#glaxnimate.io.MimeSerializer.serialize'>serialize()</a></h3>

```python
serialize(self: glaxnimate.io.MimeSerializer, arg0: List[glaxnimate.model.DocumentNode]) -> bytes
```

## glaxnimate.io.IoRegistry

<h3 id='glaxnimate.io.IoRegistry.importers'><a href='#glaxnimate.io.IoRegistry.importers'>importers()</a></h3>

```python
importers(self: glaxnimate.io.IoRegistry) -> List[glaxnimate.io.ImportExport]
```

<h3 id='glaxnimate.io.IoRegistry.exporters'><a href='#glaxnimate.io.IoRegistry.exporters'>exporters()</a></h3>

```python
exporters(self: glaxnimate.io.IoRegistry) -> List[glaxnimate.io.ImportExport]
```

<h3 id='glaxnimate.io.IoRegistry.from_extension'><a href='#glaxnimate.io.IoRegistry.from_extension'>from_extension()</a></h3>

```python
from_extension(self: glaxnimate.io.IoRegistry, arg0: str) -> glaxnimate.io.ImportExport
```

<h3 id='glaxnimate.io.IoRegistry.from_filename'><a href='#glaxnimate.io.IoRegistry.from_filename'>from_filename()</a></h3>

```python
from_filename(self: glaxnimate.io.IoRegistry, arg0: str) -> glaxnimate.io.ImportExport
```

<h3 id='glaxnimate.io.IoRegistry.serializers'><a href='#glaxnimate.io.IoRegistry.serializers'>serializers()</a></h3>

```python
serializers(self: glaxnimate.io.IoRegistry) -> List[glaxnimate.io.MimeSerializer]
```

<h3 id='glaxnimate.io.IoRegistry.serializer_from_slug'><a href='#glaxnimate.io.IoRegistry.serializer_from_slug'>serializer_from_slug()</a></h3>

```python
serializer_from_slug(self: glaxnimate.io.IoRegistry, arg0: str) -> glaxnimate.io.MimeSerializer
```

## glaxnimate.io.ImportExport

Sub classes:

* [GlaxnimateFormat](#glaxnimateioglaxnimateformat)
* [IoFormat](#glaxnimateioioformat)

Properties:

| name         | type        | notes     | docs | 
| ------------ | ----------- | --------- | ---- | 
| `name`       | `str`       | Read only |      | 
| `extensions` | `List[str]` | Read only |      | 
| `can_open`   | `bool`      | Read only |      | 
| `can_save`   | `bool`      | Read only |      | 

<h3 id='glaxnimate.io.ImportExport.can_handle_extension'><a href='#glaxnimate.io.ImportExport.can_handle_extension'>can_handle_extension()</a></h3>

Signature:

```python
can_handle_extension(self, extension: str) -> bool
```

<h3 id='glaxnimate.io.ImportExport.can_handle_filename'><a href='#glaxnimate.io.ImportExport.can_handle_filename'>can_handle_filename()</a></h3>

Signature:

```python
can_handle_filename(self, filename: str) -> bool
```

<h3 id='glaxnimate.io.ImportExport.save'><a href='#glaxnimate.io.ImportExport.save'>save()</a></h3>

Signature:

```python
save(self, document: glaxnimate.model.Document, setting_values: dict, filename: str) -> bytes
save(self, document: glaxnimate.model.Document, setting_values: dict) -> bytes
save(self, document: glaxnimate.model.Document) -> bytes
```

<h3 id='glaxnimate.io.ImportExport.name_filter'><a href='#glaxnimate.io.ImportExport.name_filter'>name_filter()</a></h3>

Signature:

```python
name_filter(self) -> str
```

<h3 id='glaxnimate.io.ImportExport.warning'><a href='#glaxnimate.io.ImportExport.warning'>warning()</a></h3>

Signature:

```python
warning(self, message: str) -> None
```

<h3 id='glaxnimate.io.ImportExport.information'><a href='#glaxnimate.io.ImportExport.information'>information()</a></h3>

Signature:

```python
information(self, message: str) -> None
```

<h3 id='glaxnimate.io.ImportExport.error'><a href='#glaxnimate.io.ImportExport.error'>error()</a></h3>

Signature:

```python
error(self, message: str) -> None
```

<h3 id='glaxnimate.io.ImportExport.progress_max_changed'><a href='#glaxnimate.io.ImportExport.progress_max_changed'>progress_max_changed()</a></h3>

```python
progress_max_changed(self: glaxnimate.io.ImportExport, arg0: int) -> None
```

<h3 id='glaxnimate.io.ImportExport.progress'><a href='#glaxnimate.io.ImportExport.progress'>progress()</a></h3>

```python
progress(self: glaxnimate.io.ImportExport, arg0: int) -> None
```

## glaxnimate.io.GlaxnimateFormat

Base classes:

* [ImportExport](#glaxnimateioimportexport)

Constants:

| name       | type                                              | value | docs | 
| ---------- | ------------------------------------------------- | ----- | ---- | 
| `instance` | [GlaxnimateFormat](#glaxnimateioglaxnimateformat) |       |      | 

## glaxnimate.io.IoFormat

Base classes:

* [ImportExport](#glaxnimateioimportexport)

# glaxnimate.model.defs

## glaxnimate.model.defs.Asset

Base classes:

* [ReferenceTarget](#glaxnimatemodelreferencetarget)

Sub classes:

* [BrushStyle](#glaxnimatemodeldefsbrushstyle)
* [GradientColors](#glaxnimatemodeldefsgradientcolors)
* [Bitmap](#glaxnimatemodeldefsbitmap)

## glaxnimate.model.defs.BrushStyle

Base classes:

* [Asset](#glaxnimatemodeldefsasset)

Sub classes:

* [NamedColor](#glaxnimatemodeldefsnamedcolor)
* [Gradient](#glaxnimatemodeldefsgradient)

## glaxnimate.model.defs.NamedColor

Base classes:

* [BrushStyle](#glaxnimatemodeldefsbrushstyle)

Properties:

| name    | type                           | notes     | docs | 
| ------- | ------------------------------ | --------- | ---- | 
| `color` | [Color](#glaxnimateutilscolor) | Read only |      | 

## glaxnimate.model.defs.GradientColors

Base classes:

* [Asset](#glaxnimatemodeldefsasset)

Properties:

| name     | type                                                 | notes     | docs | 
| -------- | ---------------------------------------------------- | --------- | ---- | 
| `colors` | List[Tuple[`float`, [Color](#glaxnimateutilscolor)]] | Read only |      | 

## glaxnimate.model.defs.Gradient.Type

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name     | type                                     | value | docs | 
| -------- | ---------------------------------------- | ----- | ---- | 
| `Linear` | [Type](#glaxnimatemodeldefsgradienttype) | `1`   |      | 
| `Radial` | [Type](#glaxnimatemodeldefsgradienttype) | `2`   |      | 

## glaxnimate.model.defs.Gradient

Base classes:

* [BrushStyle](#glaxnimatemodeldefsbrushstyle)

Properties:

| name          | type                                             | notes     | docs | 
| ------------- | ------------------------------------------------ | --------- | ---- | 
| `colors`      | [GradientColors](#glaxnimatemodelgradientcolors) | Reference |      | 
| `type`        | [Type](#glaxnimatemodeldefsgradienttype)         |           |      | 
| `start_point` | [Point](#glaxnimateutilspoint)                   | Read only |      | 
| `end_point`   | [Point](#glaxnimateutilspoint)                   | Read only |      | 
| `highlight`   | [Point](#glaxnimateutilspoint)                   | Read only |      | 

<h3 id='glaxnimate.model.defs.Gradient.radius'><a href='#glaxnimate.model.defs.Gradient.radius'>radius()</a></h3>

Signature:

```python
radius(self, t: FrameTime) -> float
```

## glaxnimate.model.defs.Bitmap

Base classes:

* [Asset](#glaxnimatemodeldefsasset)

Properties:

| name       | type    | notes     | docs | 
| ---------- | ------- | --------- | ---- | 
| `data`     | `bytes` |           |      | 
| `filename` | `str`   |           |      | 
| `format`   | `str`   | Read only |      | 
| `width`    | `int`   | Read only |      | 
| `height`   | `int`   | Read only |      | 
| `embedded` | `bool`  |           |      | 

<h3 id='glaxnimate.model.defs.Bitmap.refresh'><a href='#glaxnimate.model.defs.Bitmap.refresh'>refresh()</a></h3>

Signature:

```python
refresh(self, rebuild_embedded: bool) -> None
```

<h3 id='glaxnimate.model.defs.Bitmap.embed'><a href='#glaxnimate.model.defs.Bitmap.embed'>embed()</a></h3>

Signature:

```python
embed(self, embedded: bool) -> None
```

## glaxnimate.model.defs.Defs

Base classes:

* [Object](#glaxnimatemodelobject)

Properties:

| name              | type                                                   | notes     | docs | 
| ----------------- | ------------------------------------------------------ | --------- | ---- | 
| `colors`          | List[[NamedColor](#glaxnimatemodelnamedcolor)]         | Read only |      | 
| `images`          | List[[Bitmap](#glaxnimatemodelbitmap)]                 | Read only |      | 
| `gradient_colors` | List[[GradientColors](#glaxnimatemodelgradientcolors)] | Read only |      | 
| `gradients`       | List[[Gradient](#glaxnimatemodelgradient)]             | Read only |      | 

<h3 id='glaxnimate.model.defs.Defs.find_by_uuid'><a href='#glaxnimate.model.defs.Defs.find_by_uuid'>find_by_uuid()</a></h3>

Signature:

```python
find_by_uuid(self, n: uuid.UUID) -> glaxnimate.model.ReferenceTarget
```

<h3 id='glaxnimate.model.defs.Defs.add_color'><a href='#glaxnimate.model.defs.Defs.add_color'>add_color()</a></h3>

Signature:

```python
add_color(self, color: glaxnimate.utils.Color, name: str) -> glaxnimate.model.NamedColor
add_color(self, color: glaxnimate.utils.Color) -> glaxnimate.model.NamedColor
```

<h3 id='glaxnimate.model.defs.Defs.add_image_file'><a href='#glaxnimate.model.defs.Defs.add_image_file'>add_image_file()</a></h3>

Signature:

```python
add_image_file(self, filename: str, embed: bool) -> glaxnimate.model.Bitmap
```

<h3 id='glaxnimate.model.defs.Defs.add_image'><a href='#glaxnimate.model.defs.Defs.add_image'>add_image()</a></h3>

Signature:

```python
add_image(self, image: PIL.Image.Image, store_as: str) -> glaxnimate.model.Bitmap
add_image(self, image: PIL.Image.Image) -> glaxnimate.model.Bitmap
```

<h3 id='glaxnimate.model.defs.Defs.add_gradient'><a href='#glaxnimate.model.defs.Defs.add_gradient'>add_gradient()</a></h3>

```python
add_gradient(self: glaxnimate.model.defs.Defs, index: int = -1) -> glaxnimate.model.defs.Gradient
```

<h3 id='glaxnimate.model.defs.Defs.add_gradient_colors'><a href='#glaxnimate.model.defs.Defs.add_gradient_colors'>add_gradient_colors()</a></h3>

```python
add_gradient_colors(self: glaxnimate.model.defs.Defs, index: int = -1) -> glaxnimate.model.defs.GradientColors
```

# glaxnimate.model.shapes

## glaxnimate.model.shapes.ShapeElement

Base classes:

* [DocumentNode](#glaxnimatemodeldocumentnode)

Sub classes:

* [Shape](#glaxnimatemodelshapesshape)
* [Modifier](#glaxnimatemodelshapesmodifier)
* [Styler](#glaxnimatemodelshapesstyler)
* [Group](#glaxnimatemodelshapesgroup)
* [Image](#glaxnimatemodelshapesimage)

## glaxnimate.model.shapes.Shape

Base classes:

* [ShapeElement](#glaxnimatemodelshapesshapeelement)

Sub classes:

* [Rect](#glaxnimatemodelshapesrect)
* [Ellipse](#glaxnimatemodelshapesellipse)
* [PolyStar](#glaxnimatemodelshapespolystar)
* [Path](#glaxnimatemodelshapespath)

## glaxnimate.model.shapes.Modifier

Base classes:

* [ShapeElement](#glaxnimatemodelshapesshapeelement)

## glaxnimate.model.shapes.Styler

Base classes:

* [ShapeElement](#glaxnimatemodelshapesshapeelement)

Sub classes:

* [Fill](#glaxnimatemodelshapesfill)
* [Stroke](#glaxnimatemodelshapesstroke)

Properties:

| name      | type                                     | notes     | docs | 
| --------- | ---------------------------------------- | --------- | ---- | 
| `color`   | [Color](#glaxnimateutilscolor)           | Read only |      | 
| `opacity` | `float`                                  | Read only |      | 
| `use`     | [BrushStyle](#glaxnimatemodelbrushstyle) | Reference |      | 

## glaxnimate.model.shapes.Rect

Base classes:

* [Shape](#glaxnimatemodelshapesshape)

Properties:

| name       | type                           | notes     | docs | 
| ---------- | ------------------------------ | --------- | ---- | 
| `position` | [Point](#glaxnimateutilspoint) | Read only |      | 
| `size`     | [Size](#glaxnimateutilssize)   | Read only |      | 
| `rounded`  | `float`                        | Read only |      | 

## glaxnimate.model.shapes.Ellipse

Base classes:

* [Shape](#glaxnimatemodelshapesshape)

Properties:

| name       | type                           | notes     | docs | 
| ---------- | ------------------------------ | --------- | ---- | 
| `position` | [Point](#glaxnimateutilspoint) | Read only |      | 
| `size`     | [Size](#glaxnimateutilssize)   | Read only |      | 

## glaxnimate.model.shapes.PolyStar.StarType

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name      | type                                               | value | docs | 
| --------- | -------------------------------------------------- | ----- | ---- | 
| `Star`    | [StarType](#glaxnimatemodelshapespolystarstartype) | `1`   |      | 
| `Polygon` | [StarType](#glaxnimatemodelshapespolystarstartype) | `2`   |      | 

## glaxnimate.model.shapes.PolyStar

Base classes:

* [Shape](#glaxnimatemodelshapesshape)

Properties:

| name           | type                                               | notes     | docs | 
| -------------- | -------------------------------------------------- | --------- | ---- | 
| `type`         | [StarType](#glaxnimatemodelshapespolystarstartype) |           |      | 
| `position`     | [Point](#glaxnimateutilspoint)                     | Read only |      | 
| `outer_radius` | `float`                                            | Read only |      | 
| `inner_radius` | `float`                                            | Read only |      | 
| `angle`        | `float`                                            | Read only |      | 
| `points`       | `int`                                              | Read only |      | 

## glaxnimate.model.shapes.Path

Base classes:

* [Shape](#glaxnimatemodelshapesshape)

Properties:

| name     | type                                            | notes     | docs | 
| -------- | ----------------------------------------------- | --------- | ---- | 
| `shape`  | [Bezier](#glaxnimatemathglaxnimatebezierbezier) | Read only |      | 
| `closed` | `bool`                                          |           |      | 

## glaxnimate.model.shapes.PointType

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name          | type                                         | value | docs | 
| ------------- | -------------------------------------------- | ----- | ---- | 
| `Corner`      | [PointType](#glaxnimatemodelshapespointtype) | `0`   |      | 
| `Smooth`      | [PointType](#glaxnimatemodelshapespointtype) | `1`   |      | 
| `Symmetrical` | [PointType](#glaxnimatemodelshapespointtype) | `2`   |      | 

## glaxnimate.model.shapes.Group

Base classes:

* [ShapeElement](#glaxnimatemodelshapesshapeelement)

Sub classes:

* [Layer](#glaxnimatemodelshapeslayer)

Properties:

| name        | type                                               | notes     | docs | 
| ----------- | -------------------------------------------------- | --------- | ---- | 
| `shapes`    | List[[ShapeElement](#glaxnimatemodelshapeelement)] | Read only |      | 
| `transform` | [Transform](#glaxnimatemodeltransform)             | Read only |      | 
| `opacity`   | `float`                                            | Read only |      | 

<h3 id='glaxnimate.model.shapes.Group.add_shape'><a href='#glaxnimate.model.shapes.Group.add_shape'>add_shape()</a></h3>

```python
add_shape(self: glaxnimate.model.shapes.Group, type_name: str, index: int = -1) -> glaxnimate.model.shapes.ShapeElement
```

Adds a shape from its class name

## glaxnimate.model.shapes.Layer

Base classes:

* [Group](#glaxnimatemodelshapesgroup)

Properties:

| name         | type                                                     | notes     | docs | 
| ------------ | -------------------------------------------------------- | --------- | ---- | 
| `animation`  | [AnimationContainer](#glaxnimatemodelanimationcontainer) | Read only |      | 
| `parent`     | [Layer](#glaxnimatemodellayer)                           | Reference |      | 
| `start_time` | `float`                                                  |           |      | 
| `render`     | `bool`                                                   |           |      | 

## glaxnimate.model.shapes.Fill.Rule

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name      | type                                   | value | docs | 
| --------- | -------------------------------------- | ----- | ---- | 
| `NonZero` | [Rule](#glaxnimatemodelshapesfillrule) | `1`   |      | 
| `EvenOdd` | [Rule](#glaxnimatemodelshapesfillrule) | `0`   |      | 

## glaxnimate.model.shapes.Fill

Base classes:

* [Styler](#glaxnimatemodelshapesstyler)

Properties:

| name        | type                                   | notes | docs | 
| ----------- | -------------------------------------- | ----- | ---- | 
| `fill_rule` | [Rule](#glaxnimatemodelshapesfillrule) |       |      | 

## glaxnimate.model.shapes.Stroke.Cap

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name        | type                                   | value | docs | 
| ----------- | -------------------------------------- | ----- | ---- | 
| `ButtCap`   | [Cap](#glaxnimatemodelshapesstrokecap) | `0`   |      | 
| `RoundCap`  | [Cap](#glaxnimatemodelshapesstrokecap) | `32`  |      | 
| `SquareCap` | [Cap](#glaxnimatemodelshapesstrokecap) | `16`  |      | 

## glaxnimate.model.shapes.Stroke.Join

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name        | type                                     | value | docs | 
| ----------- | ---------------------------------------- | ----- | ---- | 
| `MiterJoin` | [Join](#glaxnimatemodelshapesstrokejoin) | `0`   |      | 
| `RoundJoin` | [Join](#glaxnimatemodelshapesstrokejoin) | `128` |      | 
| `BevelJoin` | [Join](#glaxnimatemodelshapesstrokejoin) | `64`  |      | 

## glaxnimate.model.shapes.Stroke

Base classes:

* [Styler](#glaxnimatemodelshapesstyler)

Properties:

| name          | type                                     | notes     | docs | 
| ------------- | ---------------------------------------- | --------- | ---- | 
| `width`       | `float`                                  | Read only |      | 
| `cap`         | [Cap](#glaxnimatemodelshapesstrokecap)   |           |      | 
| `join`        | [Join](#glaxnimatemodelshapesstrokejoin) |           |      | 
| `miter_limit` | `float`                                  |           |      | 

## glaxnimate.model.shapes.Image

Base classes:

* [ShapeElement](#glaxnimatemodelshapesshapeelement)

Properties:

| name        | type                                   | notes     | docs | 
| ----------- | -------------------------------------- | --------- | ---- | 
| `transform` | [Transform](#glaxnimatemodeltransform) | Read only |      | 
| `image`     | [Bitmap](#glaxnimatemodelbitmap)       | Reference |      | 

# glaxnimate.model

## glaxnimate.model.Object

Sub classes:

* [ReferenceTarget](#glaxnimatemodelreferencetarget)
* [AnimationContainer](#glaxnimatemodelanimationcontainer)
* [Transform](#glaxnimatemodeltransform)
* [Defs](#glaxnimatemodeldefsdefs)

## glaxnimate.model.UndoMacroGuard

Context manager that creates undo macros

<h3 id='glaxnimate.model.UndoMacroGuard.start'><a href='#glaxnimate.model.UndoMacroGuard.start'>start()</a></h3>

```python
start(self: glaxnimate.model.UndoMacroGuard) -> None
```

<h3 id='glaxnimate.model.UndoMacroGuard.finish'><a href='#glaxnimate.model.UndoMacroGuard.finish'>finish()</a></h3>

```python
finish(self: glaxnimate.model.UndoMacroGuard) -> None
```

## glaxnimate.model.Document

Properties:

| name                 | type                                               | notes     | docs | 
| -------------------- | -------------------------------------------------- | --------- | ---- | 
| `filename`           | `str`                                              | Read only |      | 
| `main`               | [MainComposition](#glaxnimatemodelmaincomposition) | Read only |      | 
| `current_time`       | `float`                                            |           |      | 
| `record_to_keyframe` | `bool`                                             |           |      | 
| `defs`               | [Object](#glaxnimatemodelobject)                   | Read only |      | 
| `size`               | [IntSize](#glaxnimateutilsintsize)                 | Read only |      | 
| `rect`               | [Rect](#glaxnimateutilsrect)                       | Read only |      | 

<h3 id='glaxnimate.model.Document.find_by_uuid'><a href='#glaxnimate.model.Document.find_by_uuid'>find_by_uuid()</a></h3>

Signature:

```python
find_by_uuid(self, n: uuid.UUID) -> glaxnimate.model.ReferenceTarget
```

<h3 id='glaxnimate.model.Document.find_by_type_name'><a href='#glaxnimate.model.Document.find_by_type_name'>find_by_type_name()</a></h3>

Signature:

```python
find_by_type_name(self, type_name: str) -> list
```

<h3 id='glaxnimate.model.Document.undo'><a href='#glaxnimate.model.Document.undo'>undo()</a></h3>

Signature:

```python
undo(self) -> bool
```

<h3 id='glaxnimate.model.Document.redo'><a href='#glaxnimate.model.Document.redo'>redo()</a></h3>

Signature:

```python
redo(self) -> bool
```

<h3 id='glaxnimate.model.Document.get_best_name'><a href='#glaxnimate.model.Document.get_best_name'>get_best_name()</a></h3>

Signature:

```python
get_best_name(self, node: glaxnimate.model.DocumentNode, suggestion: str) -> str
get_best_name(self, node: glaxnimate.model.DocumentNode) -> str
```

<h3 id='glaxnimate.model.Document.set_best_name'><a href='#glaxnimate.model.Document.set_best_name'>set_best_name()</a></h3>

Signature:

```python
set_best_name(self, node: glaxnimate.model.DocumentNode, suggestion: str) -> None
set_best_name(self, node: glaxnimate.model.DocumentNode) -> None
```

<h3 id='glaxnimate.model.Document.render_image'><a href='#glaxnimate.model.Document.render_image'>render_image()</a></h3>

Signature:

```python
render_image(self, time: float, size: glaxnimate.utils.IntSize) -> PIL.Image.Image
render_image(self, time: float) -> PIL.Image.Image
render_image(self) -> PIL.Image.Image
```

<h3 id='glaxnimate.model.Document.macro'><a href='#glaxnimate.model.Document.macro'>macro()</a></h3>

```python
macro(self: glaxnimate.model.Document, arg0: str) -> glaxnimate.model.UndoMacroGuard
```

Context manager to group changes into a single undo command

## glaxnimate.model.ReferenceTarget

Base classes:

* [Object](#glaxnimatemodelobject)

Sub classes:

* [DocumentNode](#glaxnimatemodeldocumentnode)
* [Asset](#glaxnimatemodeldefsasset)

Properties:

| name   | type        | notes     | docs | 
| ------ | ----------- | --------- | ---- | 
| `uuid` | `uuid.UUID` | Read only |      | 
| `name` | `str`       |           |      | 

## glaxnimate.model.DocumentNode

Base classes:

* [ReferenceTarget](#glaxnimatemodelreferencetarget)

Sub classes:

* [Composition](#glaxnimatemodelcomposition)
* [ShapeElement](#glaxnimatemodelshapesshapeelement)

Properties:

| name                | type                           | notes     | docs | 
| ------------------- | ------------------------------ | --------- | ---- | 
| `group_color`       | [Color](#glaxnimateutilscolor) |           |      | 
| `visible`           | `bool`                         |           |      | 
| `locked`            | `bool`                         |           |      | 
| `visible_recursive` | `bool`                         | Read only |      | 
| `locked_recursive`  | `bool`                         | Read only |      | 
| `selectable`        | `bool`                         | Read only |      | 

<h3 id='glaxnimate.model.DocumentNode.find_by_type_name'><a href='#glaxnimate.model.DocumentNode.find_by_type_name'>find_by_type_name()</a></h3>

Signature:

```python
find_by_type_name(self, type_name: str) -> list
```

## glaxnimate.model.AnimationContainer

Base classes:

* [Object](#glaxnimatemodelobject)

Properties:

| name           | type    | notes     | docs | 
| -------------- | ------- | --------- | ---- | 
| `first_frame`  | `float` |           |      | 
| `last_frame`   | `float` |           |      | 
| `time_visible` | `bool`  | Read only |      | 

## glaxnimate.model.Transform

Base classes:

* [Object](#glaxnimatemodelobject)

Properties:

| name           | type                                 | notes     | docs | 
| -------------- | ------------------------------------ | --------- | ---- | 
| `anchor_point` | [Point](#glaxnimateutilspoint)       | Read only |      | 
| `position`     | [Point](#glaxnimateutilspoint)       | Read only |      | 
| `scale`        | [Vector2D](#glaxnimateutilsvector2d) | Read only |      | 
| `rotation`     | `float`                              | Read only |      | 

## glaxnimate.model.Composition

Base classes:

* [DocumentNode](#glaxnimatemodeldocumentnode)

Sub classes:

* [MainComposition](#glaxnimatemodelmaincomposition)

Properties:

| name        | type                                                     | notes     | docs | 
| ----------- | -------------------------------------------------------- | --------- | ---- | 
| `animation` | [AnimationContainer](#glaxnimatemodelanimationcontainer) | Read only |      | 
| `shapes`    | List[[ShapeElement](#glaxnimatemodelshapeelement)]       | Read only |      | 

<h3 id='glaxnimate.model.Composition.add_shape'><a href='#glaxnimate.model.Composition.add_shape'>add_shape()</a></h3>

```python
add_shape(self: glaxnimate.model.Composition, type_name: str, index: int = -1) -> glaxnimate.model.ShapeElement
```

Adds a shape from its class name

## glaxnimate.model.MainComposition

Base classes:

* [Composition](#glaxnimatemodelcomposition)

Properties:

| name     | type    | notes | docs | 
| -------- | ------- | ----- | ---- | 
| `fps`    | `float` |       |      | 
| `width`  | `int`   |       |      | 
| `height` | `int`   |       |      | 

## glaxnimate.model.KeyframeTransition.Descriptive

Properties:

| name   | type  | notes     | docs | 
| ------ | ----- | --------- | ---- | 
| `name` | `str` | Read only |      | 

Constants:

| name     | type                                                         | value | docs | 
| -------- | ------------------------------------------------------------ | ----- | ---- | 
| `Hold`   | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) | `0`   |      | 
| `Linear` | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) | `1`   |      | 
| `Ease`   | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) | `2`   |      | 
| `Custom` | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) | `3`   |      | 

## glaxnimate.model.KeyframeTransition

Properties:

| name            | type                                                         | notes | docs | 
| --------------- | ------------------------------------------------------------ | ----- | ---- | 
| `hold`          | `bool`                                                       |       |      | 
| `before`        | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) |       |      | 
| `after`         | [Descriptive](#glaxnimatemodelkeyframetransitiondescriptive) |       |      | 
| `before_handle` | [Point](#glaxnimateutilspoint)                               |       |      | 
| `after_handle`  | [Point](#glaxnimateutilspoint)                               |       |      | 

<h3 id='glaxnimate.model.KeyframeTransition.set_hold'><a href='#glaxnimate.model.KeyframeTransition.set_hold'>set_hold()</a></h3>

Signature:

```python
set_hold(self, hold: bool) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.set_before'><a href='#glaxnimate.model.KeyframeTransition.set_before'>set_before()</a></h3>

Signature:

```python
set_before(self, d: Descriptive) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.set_after'><a href='#glaxnimate.model.KeyframeTransition.set_after'>set_after()</a></h3>

Signature:

```python
set_after(self, d: Descriptive) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.set_handles'><a href='#glaxnimate.model.KeyframeTransition.set_handles'>set_handles()</a></h3>

Signature:

```python
set_handles(self, before: glaxnimate.utils.Point, after: glaxnimate.utils.Point) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.set_before_handle'><a href='#glaxnimate.model.KeyframeTransition.set_before_handle'>set_before_handle()</a></h3>

Signature:

```python
set_before_handle(self, before: glaxnimate.utils.Point) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.set_after_handle'><a href='#glaxnimate.model.KeyframeTransition.set_after_handle'>set_after_handle()</a></h3>

Signature:

```python
set_after_handle(self, after: glaxnimate.utils.Point) -> None
```

<h3 id='glaxnimate.model.KeyframeTransition.lerp_factor'><a href='#glaxnimate.model.KeyframeTransition.lerp_factor'>lerp_factor()</a></h3>

Signature:

```python
lerp_factor(self, ratio: float) -> float
```

<h3 id='glaxnimate.model.KeyframeTransition.bezier_parameter'><a href='#glaxnimate.model.KeyframeTransition.bezier_parameter'>bezier_parameter()</a></h3>

Signature:

```python
bezier_parameter(self, ratio: float) -> float
```

## glaxnimate.model.Keyframe

Properties:

| name         | type                                                     | notes     | docs | 
| ------------ | -------------------------------------------------------- | --------- | ---- | 
| `time`       | `float`                                                  | Read only |      | 
| `value`      | `<type>`                                                 | Read only |      | 
| `transition` | [KeyframeTransition](#glaxnimatemodelkeyframetransition) | Read only |      | 

## glaxnimate.model.AnimatableBase

Properties:

| name             | type     | notes     | docs | 
| ---------------- | -------- | --------- | ---- | 
| `keyframe_count` | `int`    | Read only |      | 
| `value`          | `<type>` |           |      | 
| `animated`       | `bool`   | Read only |      | 

<h3 id='glaxnimate.model.AnimatableBase.value_mismatch'><a href='#glaxnimate.model.AnimatableBase.value_mismatch'>value_mismatch()</a></h3>

Signature:

```python
value_mismatch(self) -> bool
```

<h3 id='glaxnimate.model.AnimatableBase.keyframe_index'><a href='#glaxnimate.model.AnimatableBase.keyframe_index'>keyframe_index()</a></h3>

Signature:

```python
keyframe_index(self, time: FrameTime) -> int
```

<h3 id='glaxnimate.model.AnimatableBase.keyframe'><a href='#glaxnimate.model.AnimatableBase.keyframe'>keyframe()</a></h3>

```python
keyframe(self: glaxnimate.model.glaxnimate.model.AnimatableBase, arg0: float) -> glaxnimate.model.Keyframe
```

<h3 id='glaxnimate.model.AnimatableBase.set_keyframe'><a href='#glaxnimate.model.AnimatableBase.set_keyframe'>set_keyframe()</a></h3>

```python
set_keyframe(self: glaxnimate.model.glaxnimate.model.AnimatableBase, arg0: float, arg1: <type>) -> glaxnimate.model.Keyframe
```

<h3 id='glaxnimate.model.AnimatableBase.remove_keyframe_at_time'><a href='#glaxnimate.model.AnimatableBase.remove_keyframe_at_time'>remove_keyframe_at_time()</a></h3>

```python
remove_keyframe_at_time(self: glaxnimate.model.glaxnimate.model.AnimatableBase, arg0: float) -> None
```

## glaxnimate.model.Visitor

<h3 id='glaxnimate.model.Visitor.visit'><a href='#glaxnimate.model.Visitor.visit'>visit()</a></h3>

```python
visit(*args, **kwargs)
```
Overloaded function.

```python
visit(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.Document, arg1: bool) -> None
```

```python
visit(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.DocumentNode, arg1: bool) -> None
```

<h3 id='glaxnimate.model.Visitor.on_visit_document'><a href='#glaxnimate.model.Visitor.on_visit_document'>on_visit_document()</a></h3>

```python
on_visit_document(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.Document) -> None
```

<h3 id='glaxnimate.model.Visitor.on_visit_node'><a href='#glaxnimate.model.Visitor.on_visit_node'>on_visit_node()</a></h3>

```python
on_visit_node(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.DocumentNode) -> None
```

