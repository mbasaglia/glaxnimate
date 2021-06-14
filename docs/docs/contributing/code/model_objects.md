Authors: Mattia Basaglia

# Model Objects

"Model Objects" are the objects used to represent animation data internally,
there is basically a 1:1 map between the contents of a Glaxnimate json file
and these objects.

## Document Nodes

"Document Nodes" are stand alone Model Objects that are part of the document structure.

This includes Assets, Visual Nodes, Containers, etc.

## Visual Nodes

"Visual Nodes" are Document Nodes that correspond to a visual element of the
animation. These are listed in the [Layers View](../../manual/ui/docks.md#layers).

# How to define a new class

If you are defining a new class, make sure it uses all the code generation machinery.

Follows an example:

<figure>
<figcaption>example.hpp</figcaption>
```c++
namespace model {

class MyNewObject : public Object
{
    // This adds registration code for the factory and cloning functionality.
    // It's required for concrete types, not for abstract classes.
    GLAXNIMATE_OBJECT(MyNewObject)

    // You can define properties like this, they will be editable from the UI
    // the paramters to the macro are (type, name, default)
    GLAXNIMATE_PROPERTY(float, myprop, 0.2)
    // Animated properties are similar but use a different macro
    GLAXNIMATE_ANIMATABLE(float, my_animated, 0.3)

public:
    // Use the base class constructor
    using Object::Object;

    // Overriding this allows a translated name to be shown in the UI
    QString type_name_human() const override { return tr("My New Object"); }

};

// Classes that don't have to be instatiated require less machinery
class MyNewAbstractClass : public Object
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY(float, myprop, 0.2)

public:
    using Object::Object;
};

// You can also define enums, as long as they are exposed with Q_ENUM
class MyClassWithEnum : public Object
{
    Q_OBJECT

public:
    enum MyEnum
    {
        Value1,
        Value2
    }

    Q_ENUM(MyEnum)

    GLAXNIMATE_PROPERTY(MyEnum, type, Value1)


public:
    using Object::Object;
};

} // namespace model
```

<figcaption>example.cpp</figcaption>
```c++
// You always need a source file, so Qt's MOC can add its definitions
#include "example.hpp"

// This adds the definitions of the stuff declared by GLAXNIMATE_OBJECT
GLAXNIMATE_OBJECT_IMPL(model::MyNewClass)

```
</figure>

## Adding code

When you add a new class, the code-generation machinery will automatically
integrate the new class with the UI and serialization to/from Glaxnimate json.

You still need to register it in a couple places:

### Python interpreter

In `src/core/python.cpp` you have to expose classes so they are visible in scripts.

For QObject-derived classes (which include Model Objects) there's code-generation
facilities that expose properties defined by the `GLAXNIMATE_*` macros as
well as QObject-related declarations such as `Q_PROPERTY` and `Q_INVOKABLE`.

Automatic generation supports most but not all types for properties and methods.
It supports basic c++ types (like `int`, etc), common Qt types (eg: `QString`),
QObject-derived classes, and exposed enums.
Properties or methods using other types, need to be declared manually here.

If a

Example:

```c++
PYBIND11_EMBEDDED_MODULE(glaxnimate, glaxnimate_module)
{
    // ...

    // The template parameters are the class to be declared and its parent
    // The function argument is the module you want to define the class into.
    register_from_meta<model::MyNewObject, Object>(glaxnimate_module);
    // Abstract classes should also be declared,
    // otherwise their properties won't be visible from python.
    register_from_meta<model::MyNewAbstractClass, Object>(glaxnimate_module);
    // If you define enums, you need to list them like this:
    register_from_meta<model::MyClassWithEnum, Object>(glaxnimate_module, enums<model::MyClassWithEnum::MyEnum>{});
}
```

### Lottie Import/Export

Lottie serialization is semi-automatic, you need to add your class in
`core/io/lottie/lottie_private_common.hpp`.

The variable `fields` defines metadata on how to interpret the lottie json fields
and you should add your class and relevant properties in there.

Example:

```c++
const QMap<QString, QVector<FieldInfo>> fields = {
    // ...

    // Use the class name without namespace as key
    {"MyNewAbstractClass", {
        // maps the Glaxnimate property "myprop" to the json field "mp"
        FieldInfo{"mp", "myprop"},
    }},
    {"MyNewAbstractClass", {
        // fields without mappings are ignored without warning when opening files
        FieldInfo{"ignored"},
        // maps the Glaxnimate property "my_animated" to the json field "ma"
        FieldInfo{"ma", "my_animated"},
        // fields marked with Custom need to be manually read/written in the lottie code
        FieldInfo{"custom", Custom},
    }},
    {"MyClassWithEnum", {
        // If the enum values are the same in lottie you don't have to do much,
        // otherwise you need to use Custom
        FieldInfo{"type", "type"},
    }}

    // ...
};
```

Note that the order of properties is important in some lottie renderers.
Glaxnimate keeps the automatic properties in the order defined into `fields`
but care needs to be taken when setting `Custom` properties.

If you define new shapes that have a corresponding lottie type,
you need to add them to `shape_types`
which maps Glaxnimate class names (without namespace) to the lottie `ty` field
corresponding to the right object.


### SVG Import/Export

SVG rendering is done mostly manually so you'd need to add the appropriate code.

### New Shape / Layer Checklist

* Define the class, having `ShapeElement` (or the appropriate class) as parent
    * Add GLAXNIMATE_OBJECT
    * Add properties
    * Define metadata with GLAXNIMATE_OBJECT_IMPL
* If you want to add menus, inherit from the CRTP Class `StaticOverrides`
* Override the relevant methods
    * (`static_`) `tree_icon`
    * (`static_`) `type_name_human`
    * `to_path`
* Expose the class to Python
* Add editors (if needed) to `gui/graphics/create_items.cpp`.
* Implement Lottie I/O
    * Populate `io::lottie::detail::fields` in `core/io/lottie/lottie_private_common.hpp`
    * Add entry to `io::lottie::detail::shape_types` in `core/io/lottie/lottie_private_common.hpp`
    * If it has custom fields, add the relevant code to `lottie_importer.hpp` and `lottie_exporter.hpp`
* Implement SVG Export (`core/io/svg/svg_renderer.cpp`)
    * Write a conversion function
    * If it's a `Shape`, add the condition in `write_shape_shape`
    * Otherwise, add the condition in `write_shape`
* Implement SVG Import, if there is a standard SVG or Inkscape SVG analogue (`core/io/svg/svg_parser.cpp`)
    * Write a conversion function
    * If adding support for a new element, map the element name to conversion function in `io::svg::SvgParser::Private::shape_parsers`.
    * Otherwise add the relevant condition in the existing `parseshape_*` function.
