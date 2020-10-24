# glaxnimate

# glaxnimate.io

Input/Output utilities

## Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `registry` | glaxnimate.io.IoRegistry |  |  | 

## glaxnimate.io.GlaxnimateFormat

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `can_open` | bool | Read only |  | 
| `can_save` | bool | Read only |  | 
| `extensions` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 

### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `instance` | glaxnimate.io.GlaxnimateFormat |  |  | 

### glaxnimate.io.GlaxnimateFormat.can_handle_extension

can_handle_extension(self: glaxnimate.__detail.__QObject, *args) -> bool

### glaxnimate.io.GlaxnimateFormat.can_handle_filename

can_handle_filename(self: glaxnimate.__detail.__QObject, *args) -> bool

### glaxnimate.io.GlaxnimateFormat.error

error(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.io.GlaxnimateFormat.information

information(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.io.GlaxnimateFormat.name_filter

name_filter(self: glaxnimate.__detail.__QObject, *args) -> QString

### glaxnimate.io.GlaxnimateFormat.save

save(*args, **kwargs)
Overloaded function.

1. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

2. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

3. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

### glaxnimate.io.GlaxnimateFormat.warning

warning(self: glaxnimate.__detail.__QObject, *args) -> None

## glaxnimate.io.ImportExport

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `can_open` | bool | Read only |  | 
| `can_save` | bool | Read only |  | 
| `extensions` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 

### glaxnimate.io.ImportExport.can_handle_extension

can_handle_extension(self: glaxnimate.__detail.__QObject, *args) -> bool

### glaxnimate.io.ImportExport.can_handle_filename

can_handle_filename(self: glaxnimate.__detail.__QObject, *args) -> bool

### glaxnimate.io.ImportExport.error

error(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.io.ImportExport.information

information(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.io.ImportExport.name_filter

name_filter(self: glaxnimate.__detail.__QObject, *args) -> QString

### glaxnimate.io.ImportExport.save

save(*args, **kwargs)
Overloaded function.

1. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

2. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

3. save(self: glaxnimate.__detail.__QObject, *args) -> QByteArray

### glaxnimate.io.ImportExport.warning

warning(self: glaxnimate.__detail.__QObject, *args) -> None

## glaxnimate.io.IoRegistry

### glaxnimate.io.IoRegistry.exporters

exporters(self: glaxnimate.io.IoRegistry) -> List[io::ImportExport]

### glaxnimate.io.IoRegistry.from_extension

from_extension(self: glaxnimate.io.IoRegistry, arg0: QString) -> io::ImportExport

### glaxnimate.io.IoRegistry.from_filename

from_filename(self: glaxnimate.io.IoRegistry, arg0: QString) -> io::ImportExport

### glaxnimate.io.IoRegistry.importers

importers(self: glaxnimate.io.IoRegistry) -> List[io::ImportExport]

### glaxnimate.io.IoRegistry.serializer_from_slug

serializer_from_slug(self: glaxnimate.io.IoRegistry, arg0: QString) -> glaxnimate.io.MimeSerializer

### glaxnimate.io.IoRegistry.serializers

serializers(self: glaxnimate.io.IoRegistry) -> List[glaxnimate.io.MimeSerializer]

## glaxnimate.io.MimeSerializer

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `mime_types` | List[QString] | Read only |  | 
| `name` | QString | Read only |  | 
| `slug` | QString | Read only |  | 

### glaxnimate.io.MimeSerializer.serialize

serialize(self: glaxnimate.io.MimeSerializer, arg0: List[model::DocumentNode]) -> QByteArray

# glaxnimate.log

Logging utilities

## glaxnimate.log.error

error(arg0: QString) -> None

## glaxnimate.log.info

info(arg0: QString) -> None

## glaxnimate.log.warning

warning(arg0: QString) -> None

# glaxnimate.model.defs

## glaxnimate.model.defs.Asset

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

## glaxnimate.model.defs.Bitmap

### Properties

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

### glaxnimate.model.defs.Bitmap.embed

embed(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.defs.Bitmap.refresh

refresh(self: glaxnimate.__detail.__QObject, *args) -> None

## glaxnimate.model.defs.BrushStyle

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

## glaxnimate.model.defs.Defs

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | List[QVariant] | Read only |  | 
| `gradient_colors` | List[QVariant] | Read only |  | 
| `gradients` | List[QVariant] | Read only |  | 
| `images` | List[QVariant] | Read only |  | 

### glaxnimate.model.defs.Defs.add_color

add_color(*args, **kwargs)
Overloaded function.

1. add_color(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.__detail.__QObject

2. add_color(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.__detail.__QObject

### glaxnimate.model.defs.Defs.add_image

add_image(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.__detail.__QObject

### glaxnimate.model.defs.Defs.find_by_uuid

find_by_uuid(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.__detail.__QObject

## glaxnimate.model.defs.Gradient

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | glaxnimate.__detail.__QObject |  |  | 
| `end_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `highlight` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `start_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `type` | int |  |  | 
| `uuid` | QUuid | Read only |  | 

### glaxnimate.model.defs.Gradient.radius

radius(self: glaxnimate.__detail.__QObject, *args) -> float

## glaxnimate.model.defs.GradientColors

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `colors` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

## glaxnimate.model.defs.NamedColor

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

# glaxnimate.model.shapes

## glaxnimate.model.shapes.Ellipse

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `size` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Ellipse.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Fill.Rule

Members:

NonZero

EvenOdd

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `EvenOdd` | glaxnimate.model.shapes.Rule | `0` |  | 
| `NonZero` | glaxnimate.model.shapes.Rule | `1` |  | 

## glaxnimate.model.shapes.Fill

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `fill_rule` | int |  |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Fill.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Group

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Group.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Image

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `image` | glaxnimate.__detail.__QObject |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Image.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Layer

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `parent` | glaxnimate.__detail.__QObject |  |  | 
| `render` | bool |  |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `start_time` | float |  |  | 
| `transform` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Layer.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Modifier

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Modifier.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.PolyStar

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `angle` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `inner_radius` | glaxnimate.__detail.__QObject | Read only |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `outer_radius` | glaxnimate.__detail.__QObject | Read only |  | 
| `points` | glaxnimate.__detail.__QObject | Read only |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `type` | int |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.PolyStar.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Rect

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `rounded` | glaxnimate.__detail.__QObject | Read only |  | 
| `size` | glaxnimate.__detail.__QObject | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Rect.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Shape

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Shape.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.ShapeElement

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.ShapeElement.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Stroke.Cap

Members:

ButtCap

RoundCap

SquareCap

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `ButtCap` | glaxnimate.model.shapes.Cap | `0` |  | 
| `RoundCap` | glaxnimate.model.shapes.Cap | `32` |  | 
| `SquareCap` | glaxnimate.model.shapes.Cap | `16` |  | 

## glaxnimate.model.shapes.Stroke.Join

Members:

MiterJoin

RoundJoin

BevelJoin

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | str | Read only | name(self: handle) -> str | 

### Constants

| name | type | value | docs | 
| ---- | ---- | ---- | ---- |
| `BevelJoin` | glaxnimate.model.shapes.Join | `64` |  | 
| `MiterJoin` | glaxnimate.model.shapes.Join | `0` |  | 
| `RoundJoin` | glaxnimate.model.shapes.Join | `128` |  | 

## glaxnimate.model.shapes.Stroke

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `cap` | int |  |  | 
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `join` | int |  |  | 
| `locked` | bool |  |  | 
| `miter_limit` | float |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `width` | glaxnimate.__detail.__QObject | Read only |  | 

### glaxnimate.model.shapes.Stroke.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.shapes.Styler

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `color` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `opacity` | glaxnimate.__detail.__QObject | Read only |  | 
| `use` | glaxnimate.__detail.__QObject |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.shapes.Styler.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

# glaxnimate.model

## glaxnimate.model.AnimatableBase

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animated` | bool | Read only |  | 
| `keyframe_count` | int | Read only |  | 
| `value` | QVariant |  |  | 

### glaxnimate.model.AnimatableBase.keyframe

keyframe(self: glaxnimate.model.AnimatableBase, arg0: float) -> glaxnimate.model.Keyframe

### glaxnimate.model.AnimatableBase.keyframe_index

keyframe_index(self: glaxnimate.__detail.__QObject, *args) -> int

### glaxnimate.model.AnimatableBase.remove_keyframe_at_time

remove_keyframe_at_time(self: glaxnimate.model.AnimatableBase, arg0: float) -> None

### glaxnimate.model.AnimatableBase.set_keyframe

set_keyframe(self: glaxnimate.model.AnimatableBase, arg0: float, arg1: QVariant) -> glaxnimate.model.Keyframe

### glaxnimate.model.AnimatableBase.value_mismatch

value_mismatch(self: glaxnimate.__detail.__QObject, *args) -> bool

## glaxnimate.model.AnimationContainer

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `first_frame` | int |  |  | 
| `last_frame` | int |  |  | 
| `time_visible` | bool | Read only |  | 

## glaxnimate.model.Composition

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.Composition.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.Document

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `current_time` | float |  |  | 
| `defs` | glaxnimate.__detail.__QObject | Read only |  | 
| `filename` | QString | Read only |  | 
| `main` | glaxnimate.__detail.__QObject | Read only |  | 
| `record_to_keyframe` | bool |  |  | 

### glaxnimate.model.Document.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

### glaxnimate.model.Document.find_by_uuid

find_by_uuid(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.__detail.__QObject

### glaxnimate.model.Document.get_best_name

get_best_name(*args, **kwargs)
Overloaded function.

1. get_best_name(self: glaxnimate.__detail.__QObject, *args) -> QString

2. get_best_name(self: glaxnimate.__detail.__QObject, *args) -> QString

### glaxnimate.model.Document.macro

macro(self: glaxnimate.model.Document, arg0: QString) -> glaxnimate.__detail.UndoMacroGuard

### glaxnimate.model.Document.rect

rect(self: glaxnimate.__detail.__QObject, *args) -> QRectF

### glaxnimate.model.Document.redo

redo(self: glaxnimate.__detail.__QObject, *args) -> bool

### glaxnimate.model.Document.set_best_name

set_best_name(*args, **kwargs)
Overloaded function.

1. set_best_name(self: glaxnimate.__detail.__QObject, *args) -> None

2. set_best_name(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.Document.size

size(self: glaxnimate.__detail.__QObject, *args) -> glaxnimate.utils.IntSize

### glaxnimate.model.Document.undo

undo(self: glaxnimate.__detail.__QObject, *args) -> bool

## glaxnimate.model.DocumentNode

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `group_color` | glaxnimate.utils.Color |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 

### glaxnimate.model.DocumentNode.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.Keyframe

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `time` | float | Read only |  | 
| `transition` | glaxnimate.model.KeyframeTransition | Read only |  | 
| `value` | QVariant | Read only |  | 

## glaxnimate.model.KeyframeTransition

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `after` | int |  |  | 
| `after_handle` | glaxnimate.utils.Point |  |  | 
| `before` | int |  |  | 
| `before_handle` | glaxnimate.utils.Point |  |  | 
| `hold` | bool |  |  | 

### glaxnimate.model.KeyframeTransition.bezier_parameter

bezier_parameter(self: glaxnimate.__detail.__QObject, *args) -> float

### glaxnimate.model.KeyframeTransition.lerp_factor

lerp_factor(self: glaxnimate.__detail.__QObject, *args) -> float

### glaxnimate.model.KeyframeTransition.set_after

set_after(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.KeyframeTransition.set_after_handle

set_after_handle(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.KeyframeTransition.set_before

set_before(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.KeyframeTransition.set_before_handle

set_before_handle(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.KeyframeTransition.set_handles

set_handles(self: glaxnimate.__detail.__QObject, *args) -> None

### glaxnimate.model.KeyframeTransition.set_hold

set_hold(self: glaxnimate.__detail.__QObject, *args) -> None

## glaxnimate.model.MainComposition

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `animation` | glaxnimate.__detail.__QObject | Read only |  | 
| `fps` | float |  |  | 
| `group_color` | glaxnimate.utils.Color |  |  | 
| `height` | int |  |  | 
| `locked` | bool |  |  | 
| `name` | QString |  |  | 
| `shapes` | List[QVariant] | Read only |  | 
| `uuid` | QUuid | Read only |  | 
| `visible` | bool |  |  | 
| `width` | int |  |  | 

### glaxnimate.model.MainComposition.find_by_type_name

find_by_type_name(self: glaxnimate.__detail.__QObject, *args) -> List[QVariant]

## glaxnimate.model.Object

## glaxnimate.model.ReferenceTarget

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `name` | QString |  |  | 
| `uuid` | QUuid | Read only |  | 

## glaxnimate.model.Transform

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `anchor_point` | glaxnimate.__detail.__QObject | Read only |  | 
| `position` | glaxnimate.__detail.__QObject | Read only |  | 
| `rotation` | glaxnimate.__detail.__QObject | Read only |  | 
| `scale` | glaxnimate.__detail.__QObject | Read only |  | 

## glaxnimate.model.Visitor

### glaxnimate.model.Visitor.on_visit_document

on_visit_document(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.Document) -> None

### glaxnimate.model.Visitor.on_visit_node

on_visit_node(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.DocumentNode) -> None

### glaxnimate.model.Visitor.visit

visit(*args, **kwargs)
Overloaded function.

1. visit(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.Document) -> None

2. visit(self: glaxnimate.model.Visitor, arg0: glaxnimate.model.DocumentNode) -> None

# glaxnimate.utils

## glaxnimate.utils.Color

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `blue` | int |  |  | 
| `green` | int |  |  | 
| `name` | QString |  |  | 
| `red` | int |  |  | 

## glaxnimate.utils.IntSize

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `height` | float |  |  | 
| `width` | float |  |  | 

## glaxnimate.utils.Point

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `length` | float | Read only |  | 
| `x` | float |  |  | 
| `y` | float |  |  | 

## glaxnimate.utils.Size

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `height` | float |  |  | 
| `width` | float |  |  | 

## glaxnimate.utils.Vector2D

### Properties

| name | type | notes | docs | 
| ---- | ---- | ---- | ---- |
| `x` | float |  |  | 
| `y` | float |  |  | 

### glaxnimate.utils.Vector2D.length

length(self: glaxnimate.utils.Vector2D) -> float

### glaxnimate.utils.Vector2D.length_squared

length_squared(self: glaxnimate.utils.Vector2D) -> float

### glaxnimate.utils.Vector2D.normalize

normalize(self: glaxnimate.utils.Vector2D) -> None

### glaxnimate.utils.Vector2D.normalized

normalized(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Vector2D

### glaxnimate.utils.Vector2D.to_point

to_point(self: glaxnimate.utils.Vector2D) -> glaxnimate.utils.Point

