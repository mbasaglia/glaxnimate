# glaxnimate

# glaxnimate.io

Input/Output utilities

## Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `registry` | glaxnimate.io.IoRegistry |  |  | 

## Classes

### glaxnimate.io.GlaxnimateFormat

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `can_open` | bool | Read only |  | 
| `can_save` | bool | Read only |  | 
| `extensions` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 

#### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `instance` | glaxnimate.io.GlaxnimateFormat |  |  | 

#### Functions

<h4 id='glaxnimate.io.GlaxnimateFormat.can_handle_extension'><a href='#glaxnimate.io.GlaxnimateFormat.can_handle_extension'>can_handle_extension()</a></h4>

```python
can_handle_extension(self, *args) -> bool
```

<h4 id='glaxnimate.io.GlaxnimateFormat.can_handle_filename'><a href='#glaxnimate.io.GlaxnimateFormat.can_handle_filename'>can_handle_filename()</a></h4>

```python
can_handle_filename(self, *args) -> bool
```

<h4 id='glaxnimate.io.GlaxnimateFormat.error'><a href='#glaxnimate.io.GlaxnimateFormat.error'>error()</a></h4>

```python
error(self, *args) -> None
```

<h4 id='glaxnimate.io.GlaxnimateFormat.information'><a href='#glaxnimate.io.GlaxnimateFormat.information'>information()</a></h4>

```python
information(self, *args) -> None
```

<h4 id='glaxnimate.io.GlaxnimateFormat.name_filter'><a href='#glaxnimate.io.GlaxnimateFormat.name_filter'>name_filter()</a></h4>

```python
name_filter(self, *args) -> QString
```

<h4 id='glaxnimate.io.GlaxnimateFormat.save'><a href='#glaxnimate.io.GlaxnimateFormat.save'>save()</a></h4>

```python
save(*args, **kwargs)
```
Overloaded function.

```python
save(self, *args) -> QByteArray
```

```python
save(self, *args) -> QByteArray
```

```python
save(self, *args) -> QByteArray
```

<h4 id='glaxnimate.io.GlaxnimateFormat.warning'><a href='#glaxnimate.io.GlaxnimateFormat.warning'>warning()</a></h4>

```python
warning(self, *args) -> None
```

### glaxnimate.io.ImportExport

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `can_open` | bool | Read only |  | 
| `can_save` | bool | Read only |  | 
| `extensions` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 

#### Functions

<h4 id='glaxnimate.io.ImportExport.can_handle_extension'><a href='#glaxnimate.io.ImportExport.can_handle_extension'>can_handle_extension()</a></h4>

```python
can_handle_extension(self, *args) -> bool
```

<h4 id='glaxnimate.io.ImportExport.can_handle_filename'><a href='#glaxnimate.io.ImportExport.can_handle_filename'>can_handle_filename()</a></h4>

```python
can_handle_filename(self, *args) -> bool
```

<h4 id='glaxnimate.io.ImportExport.error'><a href='#glaxnimate.io.ImportExport.error'>error()</a></h4>

```python
error(self, *args) -> None
```

<h4 id='glaxnimate.io.ImportExport.information'><a href='#glaxnimate.io.ImportExport.information'>information()</a></h4>

```python
information(self, *args) -> None
```

<h4 id='glaxnimate.io.ImportExport.name_filter'><a href='#glaxnimate.io.ImportExport.name_filter'>name_filter()</a></h4>

```python
name_filter(self, *args) -> QString
```

<h4 id='glaxnimate.io.ImportExport.save'><a href='#glaxnimate.io.ImportExport.save'>save()</a></h4>

```python
save(*args, **kwargs)
```
Overloaded function.

```python
save(self, *args) -> QByteArray
```

```python
save(self, *args) -> QByteArray
```

```python
save(self, *args) -> QByteArray
```

<h4 id='glaxnimate.io.ImportExport.warning'><a href='#glaxnimate.io.ImportExport.warning'>warning()</a></h4>

```python
warning(self, *args) -> None
```

### glaxnimate.io.IoRegistry

#### Functions

<h4 id='glaxnimate.io.IoRegistry.exporters'><a href='#glaxnimate.io.IoRegistry.exporters'>exporters()</a></h4>

```python
exporters(self: glaxnimate.io.IoRegistry) -> List[io::ImportExport]
```

<h4 id='glaxnimate.io.IoRegistry.from_extension'><a href='#glaxnimate.io.IoRegistry.from_extension'>from_extension()</a></h4>

```python
from_extension(self: glaxnimate.io.IoRegistry, arg0: QString) -> io::ImportExport
```

<h4 id='glaxnimate.io.IoRegistry.from_filename'><a href='#glaxnimate.io.IoRegistry.from_filename'>from_filename()</a></h4>

```python
from_filename(self: glaxnimate.io.IoRegistry, arg0: QString) -> io::ImportExport
```

<h4 id='glaxnimate.io.IoRegistry.importers'><a href='#glaxnimate.io.IoRegistry.importers'>importers()</a></h4>

```python
importers(self: glaxnimate.io.IoRegistry) -> List[io::ImportExport]
```

<h4 id='glaxnimate.io.IoRegistry.serializer_from_slug'><a href='#glaxnimate.io.IoRegistry.serializer_from_slug'>serializer_from_slug()</a></h4>

```python
serializer_from_slug(self: glaxnimate.io.IoRegistry, arg0: QString) -> glaxnimate.io.MimeSerializer
```

<h4 id='glaxnimate.io.IoRegistry.serializers'><a href='#glaxnimate.io.IoRegistry.serializers'>serializers()</a></h4>

```python
serializers(self: glaxnimate.io.IoRegistry) -> List[glaxnimate.io.MimeSerializer]
```

### glaxnimate.io.MimeSerializer

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `mime_types` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 
| `slug` | QString | Read only |  | 

#### Functions

<h4 id='glaxnimate.io.MimeSerializer.serialize'><a href='#glaxnimate.io.MimeSerializer.serialize'>serialize()</a></h4>

```python
serialize(self: glaxnimate.io.MimeSerializer, arg0: List[model::DocumentNode]) -> QByteArray
```

# glaxnimate.log

Logging utilities

## Functions

<h2 id='glaxnimate.log.error'><a href='#glaxnimate.log.error'>error()</a></h2>

```python
error(arg0: QString) -> None
```

<h2 id='glaxnimate.log.info'><a href='#glaxnimate.log.info'>info()</a></h2>

```python
info(arg0: QString) -> None
```

<h2 id='glaxnimate.log.warning'><a href='#glaxnimate.log.warning'>warning()</a></h2>

```python
warning(arg0: QString) -> None
```

# glaxnimate.model.defs

## Classes

### glaxnimate.model.defs.Asset

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

### glaxnimate.model.defs.Bitmap

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `data` | QByteArray |  |  | 
| `embedded` | bool |  |  | 
| `filename` | QString |  |  | 
| `format` | QString | Read only |  | 
| `height` | int | Read only |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 
| `width` | int | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.defs.Bitmap.embed'><a href='#glaxnimate.model.defs.Bitmap.embed'>embed()</a></h4>

```python
embed(self, *args) -> None
```

<h4 id='glaxnimate.model.defs.Bitmap.refresh'><a href='#glaxnimate.model.defs.Bitmap.refresh'>refresh()</a></h4>

```python
refresh(self, *args) -> None
```

### glaxnimate.model.defs.BrushStyle

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

### glaxnimate.model.defs.Defs

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | List[QVariant] | Read only |  | 
| `gradient_colors` | List[QVariant] | Read only |  | 
| `gradients` | List[QVariant] | Read only |  | 
| `images` | List[QVariant] | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.defs.Defs.add_color'><a href='#glaxnimate.model.defs.Defs.add_color'>add_color()</a></h4>

```python
add_color(*args, **kwargs)
```
Overloaded function.

```python
add_color(self, *args) -> glaxnimate.__detail.__QObject
```

```python
add_color(self, *args) -> glaxnimate.__detail.__QObject
```

<h4 id='glaxnimate.model.defs.Defs.add_image'><a href='#glaxnimate.model.defs.Defs.add_image'>add_image()</a></h4>

```python
add_image(self, *args) -> glaxnimate.__detail.__QObject
```

<h4 id='glaxnimate.model.defs.Defs.find_by_uuid'><a href='#glaxnimate.model.defs.Defs.find_by_uuid'>find_by_uuid()</a></h4>

```python
find_by_uuid(self, *args) -> glaxnimate.__detail.__QObject
```

### glaxnimate.model.defs.Gradient

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | glaxnimate.__detail.__QObject |  |  | 
| `end_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `highlight` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `start_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `type` | int |  |  | 
| `uuid` | QUuid | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.defs.Gradient.radius'><a href='#glaxnimate.model.defs.Gradient.radius'>radius()</a></h4>

```python
radius(self, *args) -> float
```

### glaxnimate.model.defs.GradientColors

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

### glaxnimate.model.defs.NamedColor

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

# glaxnimate.model.shapes

## Classes

### glaxnimate.model.shapes.Ellipse

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `size` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Ellipse.find_by_type_name'><a href='#glaxnimate.model.shapes.Ellipse.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Fill.Rule

Members:

NonZero

EvenOdd

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

#### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `EvenOdd` | glaxnimate.model.shapes.Rule | `0` |  | 
| `NonZero` | glaxnimate.model.shapes.Rule | `1` |  | 

### glaxnimate.model.shapes.Fill

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `fill_rule` | int |  |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Fill.find_by_type_name'><a href='#glaxnimate.model.shapes.Fill.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Group

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Group.find_by_type_name'><a href='#glaxnimate.model.shapes.Group.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Image

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `image` | glaxnimate.__detail.__QObject |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Image.find_by_type_name'><a href='#glaxnimate.model.shapes.Image.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Layer

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `parent` | glaxnimate.__detail.__QObject |  |  | 
| `render` | bool |  |  | 
| `selectable` | bool | Read only |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `start_time` | float |  |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Layer.find_by_type_name'><a href='#glaxnimate.model.shapes.Layer.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Modifier

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Modifier.find_by_type_name'><a href='#glaxnimate.model.shapes.Modifier.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.PolyStar

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `angle` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `inner_radius` | glaxnimate.__detail.__QObject | Read only |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `outer_radius` | glaxnimate.__detail.__QObject | Read only |  | 
| `points` | glaxnimate.__detail.__QObject | Read only |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `type` | int |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.PolyStar.find_by_type_name'><a href='#glaxnimate.model.shapes.PolyStar.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Rect

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `rounded` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `size` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Rect.find_by_type_name'><a href='#glaxnimate.model.shapes.Rect.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Shape

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Shape.find_by_type_name'><a href='#glaxnimate.model.shapes.Shape.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.ShapeElement

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.ShapeElement.find_by_type_name'><a href='#glaxnimate.model.shapes.ShapeElement.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Stroke.Cap

Members:

ButtCap

RoundCap

SquareCap

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

#### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `ButtCap` | glaxnimate.model.shapes.Cap | `0` |  | 
| `RoundCap` | glaxnimate.model.shapes.Cap | `32` |  | 
| `SquareCap` | glaxnimate.model.shapes.Cap | `16` |  | 

### glaxnimate.model.shapes.Stroke.Join

Members:

MiterJoin

RoundJoin

BevelJoin

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

#### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `BevelJoin` | glaxnimate.model.shapes.Join | `64` |  | 
| `MiterJoin` | glaxnimate.model.shapes.Join | `0` |  | 
| `RoundJoin` | glaxnimate.model.shapes.Join | `128` |  | 

### glaxnimate.model.shapes.Stroke

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `cap` | int |  |  | 
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `join` | int |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `miter_limit` | float |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 
| `width` | glaxnimate.__detail.__QObject | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Stroke.find_by_type_name'><a href='#glaxnimate.model.shapes.Stroke.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.shapes.Styler

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `selectable` | bool | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.shapes.Styler.find_by_type_name'><a href='#glaxnimate.model.shapes.Styler.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

# glaxnimate.model

## Classes

### glaxnimate.model.AnimatableBase

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animated` | bool | Read only |  | 
| `keyframe_count` | int | Read only |  | 
| `value` | QVariant |  |  | 

#### Functions

<h4 id='glaxnimate.model.AnimatableBase.keyframe'><a href='#glaxnimate.model.AnimatableBase.keyframe'>keyframe()</a></h4>

```python
keyframe(self: glaxnimate.model.AnimatableBase, arg0: float) -> glaxnimate.model.Keyframe
```

<h4 id='glaxnimate.model.AnimatableBase.keyframe_index'><a href='#glaxnimate.model.AnimatableBase.keyframe_index'>keyframe_index()</a></h4>

```python
keyframe_index(self, *args) -> int
```

<h4 id='glaxnimate.model.AnimatableBase.remove_keyframe_at_time'><a href='#glaxnimate.model.AnimatableBase.remove_keyframe_at_time'>remove_keyframe_at_time()</a></h4>

```python
remove_keyframe_at_time(self: glaxnimate.model.AnimatableBase, arg0: float) -> None
```

<h4 id='glaxnimate.model.AnimatableBase.set_keyframe'><a href='#glaxnimate.model.AnimatableBase.set_keyframe'>set_keyframe()</a></h4>

```python
set_keyframe(self: glaxnimate.model.AnimatableBase, arg0: float, arg1: QVariant) -> glaxnimate.model.Keyframe
```

<h4 id='glaxnimate.model.AnimatableBase.value_mismatch'><a href='#glaxnimate.model.AnimatableBase.value_mismatch'>value_mismatch()</a></h4>

```python
value_mismatch(self, *args) -> bool
```

### glaxnimate.model.AnimationContainer

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `first_frame` | int |  |  | 
| `last_frame` | int |  |  | 
| `time_visible` | bool | Read only |  | 

### glaxnimate.model.Composition

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.Composition.find_by_type_name'><a href='#glaxnimate.model.Composition.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.Document

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `current_time` | float |  |  | 
| `defs` | glaxnimate.__detail.__QObject | Read only |  | 
| `filename` | QString | Read only |  | 
| `main` | glaxnimate.__detail.__QObject | Read only |  | 
| `record_to_keyframe` | bool |  |  | 

#### Functions

<h4 id='glaxnimate.model.Document.find_by_type_name'><a href='#glaxnimate.model.Document.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

<h4 id='glaxnimate.model.Document.find_by_uuid'><a href='#glaxnimate.model.Document.find_by_uuid'>find_by_uuid()</a></h4>

```python
find_by_uuid(self, *args) -> glaxnimate.__detail.__QObject
```

<h4 id='glaxnimate.model.Document.get_best_name'><a href='#glaxnimate.model.Document.get_best_name'>get_best_name()</a></h4>

```python
get_best_name(*args, **kwargs)
```
Overloaded function.

```python
get_best_name(self, *args) -> QString
```

```python
get_best_name(self, *args) -> QString
```

<h4 id='glaxnimate.model.Document.macro'><a href='#glaxnimate.model.Document.macro'>macro()</a></h4>

```python
macro(self: glaxnimate.model.Document, arg0: QString) -> glaxnimate.__detail.UndoMacroGuard
```

<h4 id='glaxnimate.model.Document.rect'><a href='#glaxnimate.model.Document.rect'>rect()</a></h4>

```python
rect(self, *args) -> QRectF
```

<h4 id='glaxnimate.model.Document.redo'><a href='#glaxnimate.model.Document.redo'>redo()</a></h4>

```python
redo(self, *args) -> bool
```

<h4 id='glaxnimate.model.Document.set_best_name'><a href='#glaxnimate.model.Document.set_best_name'>set_best_name()</a></h4>

```python
set_best_name(*args, **kwargs)
```
Overloaded function.

```python
set_best_name(self, *args) -> None
```

```python
set_best_name(self, *args) -> None
```

<h4 id='glaxnimate.model.Document.size'><a href='#glaxnimate.model.Document.size'>size()</a></h4>

```python
size(self, *args) -> glaxnimate.utils.IntSize
```

<h4 id='glaxnimate.model.Document.undo'><a href='#glaxnimate.model.Document.undo'>undo()</a></h4>

```python
undo(self, *args) -> bool
```

### glaxnimate.model.DocumentNode

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 

#### Functions

<h4 id='glaxnimate.model.DocumentNode.find_by_type_name'><a href='#glaxnimate.model.DocumentNode.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.Keyframe

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `time` | float | Read only |  | 
| `transition` | glaxnimate.model.KeyframeTransition | Read only |  | 
| `value` | QVariant | Read only |  | 

### glaxnimate.model.KeyframeTransition

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `after` | int |  |  | 
| `after_handle` | glaxnimate.utils.Point |  |  | 
| `before` | int |  |  | 
| `before_handle` | glaxnimate.utils.Point |  |  | 
| `hold` | bool |  |  | 

#### Functions

<h4 id='glaxnimate.model.KeyframeTransition.bezier_parameter'><a href='#glaxnimate.model.KeyframeTransition.bezier_parameter'>bezier_parameter()</a></h4>

```python
bezier_parameter(self, *args) -> float
```

<h4 id='glaxnimate.model.KeyframeTransition.lerp_factor'><a href='#glaxnimate.model.KeyframeTransition.lerp_factor'>lerp_factor()</a></h4>

```python
lerp_factor(self, *args) -> float
```

<h4 id='glaxnimate.model.KeyframeTransition.set_after'><a href='#glaxnimate.model.KeyframeTransition.set_after'>set_after()</a></h4>

```python
set_after(self, *args) -> None
```

<h4 id='glaxnimate.model.KeyframeTransition.set_after_handle'><a href='#glaxnimate.model.KeyframeTransition.set_after_handle'>set_after_handle()</a></h4>

```python
set_after_handle(self, *args) -> None
```

<h4 id='glaxnimate.model.KeyframeTransition.set_before'><a href='#glaxnimate.model.KeyframeTransition.set_before'>set_before()</a></h4>

```python
set_before(self, *args) -> None
```

<h4 id='glaxnimate.model.KeyframeTransition.set_before_handle'><a href='#glaxnimate.model.KeyframeTransition.set_before_handle'>set_before_handle()</a></h4>

```python
set_before_handle(self, *args) -> None
```

<h4 id='glaxnimate.model.KeyframeTransition.set_handles'><a href='#glaxnimate.model.KeyframeTransition.set_handles'>set_handles()</a></h4>

```python
set_handles(self, *args) -> None
```

<h4 id='glaxnimate.model.KeyframeTransition.set_hold'><a href='#glaxnimate.model.KeyframeTransition.set_hold'>set_hold()</a></h4>

```python
set_hold(self, *args) -> None
```

### glaxnimate.model.MainComposition

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `fps` | float |  |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `height` | int |  |  | 
| `locked` | bool |  |  | 
| `locked_recursive` | bool | Read only |  | 
| `name` | QString |  |  | 
| `selectable` | bool | Read only |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `visible_recursive` | bool | Read only |  | 
| `width` | int |  |  | 

#### Functions

<h4 id='glaxnimate.model.MainComposition.find_by_type_name'><a href='#glaxnimate.model.MainComposition.find_by_type_name'>find_by_type_name()</a></h4>

```python
find_by_type_name(self, *args) -> List[QVariant]
```

### glaxnimate.model.Object

### glaxnimate.model.ReferenceTarget

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

### glaxnimate.model.Transform

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `anchor_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `rotation` | glaxnimate.__detail.__QObject | Read only |  | 
| `scale` | glaxnimate.__detail.__QObject | Read only |  | 

### glaxnimate.model.Visitor

#### Functions

<h4 id='glaxnimate.model.Visitor.on_visit_document'><a href='#glaxnimate.model.Visitor.on_visit_document'>on_visit_document()</a></h4>

```python
on_visit_document(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.Document) -> None
```

<h4 id='glaxnimate.model.Visitor.on_visit_node'><a href='#glaxnimate.model.Visitor.on_visit_node'>on_visit_node()</a></h4>

```python
on_visit_node(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.DocumentNode) -> None
```

<h4 id='glaxnimate.model.Visitor.visit'><a href='#glaxnimate.model.Visitor.visit'>visit()</a></h4>

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

# glaxnimate.utils

## Classes

### glaxnimate.utils.Color

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `blue` | int |  |  | 
| `green` | int |  |  | 
| `name` | QString |  |  | 
| `red` | int |  |  | 

### glaxnimate.utils.IntSize

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `height` | float |  |  | 
| `width` | float |  |  | 

### glaxnimate.utils.Point

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `length` | float | Read only |  | 
| `x` | float |  |  | 
| `y` | float |  |  | 

### glaxnimate.utils.Size

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `height` | float |  |  | 
| `width` | float |  |  | 

### glaxnimate.utils.Vector2D

#### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `x` | float |  |  | 
| `y` | float |  |  | 

#### Functions

<h4 id='glaxnimate.utils.Vector2D.length'><a href='#glaxnimate.utils.Vector2D.length'>length()</a></h4>

```python
length(self: glaxnimate.utils.Vector2D) -> float
```

<h4 id='glaxnimate.utils.Vector2D.length_squared'><a href='#glaxnimate.utils.Vector2D.length_squared'>length_squared()</a></h4>

```python
length_squared(self: glaxnimate.utils.Vector2D) -> float
```

<h4 id='glaxnimate.utils.Vector2D.normalize'><a href='#glaxnimate.utils.Vector2D.normalize'>normalize()</a></h4>

```python
normalize(self: glaxnimate.utils.Vector2D) -> None
```

<h4 id='glaxnimate.utils.Vector2D.normalized'><a href='#glaxnimate.utils.Vector2D.normalized'>normalized()</a></h4>

```python
normalized(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Vector2D
```

<h4 id='glaxnimate.utils.Vector2D.to_point'><a href='#glaxnimate.utils.Vector2D.to_point'>to_point()</a></h4>

```python
to_point(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Point
```

