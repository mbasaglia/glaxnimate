#pragma once

#include "rive_stream.hpp"
#include "type_def.hpp"

#include "rive_format.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

namespace glaxnimate::io::rive {

class RiveLoader
{
public:
    struct ObjectPointer
    {
        ObjectPointer() = default;

        ObjectPointer(Object* first, Object* last)
            : object(first),
              children(object),
              child_count(last - first - 1)
        {}

        Object* operator->() const { return object; }

        Object* object = nullptr;
        Object* children = nullptr;
        Identifier child_count = 0;
    };

    struct LoadCotext
    {
        ObjectPointer point_to(Object* iterator)
        {
            return ObjectPointer(iterator, &objects.back());
        }


        Object* artboard_child(Identifier id) const
        {
            if ( artboard.object && id < artboard.child_count )
                return artboard.children + id;
            return nullptr;
        }

        std::vector<Object> objects;
        ObjectPointer artboard;
        Object* keyed_object = nullptr;
        PropertyAnimation* keyed_property = nullptr;
    };

    RiveLoader(RiveStream& stream, RiveFormat* format)
        : document(nullptr),
        stream(stream),
        format(format)
    {
    }


    std::vector<Object> load_object_list()
    {
        extra_props = stream.read_property_table();
        if ( stream.has_error() )
            return {};

        std::vector<Object> objects;
        while ( !stream.has_error() && !stream.eof() )
            objects.emplace_back(read_object());
        return objects;
    }

    bool load_document(model::Document* document)
    {
        this->document = document;

        extra_props = stream.read_property_table();

        if ( stream.has_error() )
        {
            format->error(QObject::tr("Could not read property table"));
            return false;
        }

        LoadCotext context;
        while ( !stream.has_error() && !stream.eof() )
            context.objects.emplace_back(read_object());

        for ( auto& object : context.objects )
            preprocess_object(&object, context);


        for ( auto& object : context.objects )
            process_object(&object);

        return true;
    }

    bool gather_definitions(TypeId type_id, Object& object)
    {
        auto it = defined_objects.find(type_id);
        if ( it == defined_objects.end() )
        {
            fail(QObject::tr("Unknown object of type %1").arg(int(type_id)));
            return {};
        }

        const auto& def = it->second;

        object.definitions.push_back(&def);

        if ( def.extends != TypeId::NoType )
        {
            if ( !gather_definitions(def.extends, object) )
                return false;
        }

        object.property_definitions.insert(def.properties.begin(), def.properties.end());
        return true;
    }

    Object read_object()
    {
        auto type_id = TypeId(stream.read_varuint());
        if ( stream.has_error() )
        {
            fail(QObject::tr("Could not load object type ID"));
            return {};
        }

        Object obj;
        obj.type_id = type_id;

        if ( !gather_definitions(type_id, obj) )
            return {};

        while ( true )
        {
            Identifier prop_id = stream.read_varuint();
            if ( stream.has_error() )
            {
                fail(QObject::tr("Could not load property ID in %1 (%2)")
                    .arg(int(type_id)).arg(obj.definitions[0]->name));
                return {};
            }

            if ( prop_id == 0 )
                break;

            auto prop_def = obj.property_definitions.find(prop_id);
            if ( prop_def == obj.property_definitions.end() )
            {
                auto unknown_it = extra_props.find(prop_id);
                if ( unknown_it == extra_props.end() )
                {
                    fail(QObject::tr("Unknown property %1 of %2 (%3)")
                        .arg(prop_id).arg(int(type_id)).arg(obj.definitions[0]->name));
                    return {};
                }
                else
                {
                    format->warning(QObject::tr("Skipping unknown property %1 of %2 (%3)")
                            .arg(prop_id).arg(int(type_id)).arg(obj.definitions[0]->name));
                }
            }
            else
            {
                obj.properties[prop_def->second.name] = read_property_value(prop_def->second.type);
                if ( stream.has_error() )
                {
                    fail(QObject::tr("Error loading property %1 (%2) of %3 (%4)")
                        .arg(prop_id).arg(prop_def->second.name).arg(int(type_id)).arg(obj.definitions[0]->name));
                    return {};
                }
            }
        }

        qDebug() << obj.definitions[0]->name << obj.properties;
        return obj;
    }

    void fail(const QString& message)
    {
        format->error(message);
        failed = true;
    }

    QVariant read_property_value(PropertyType type)
    {
        switch ( type )
        {
            case PropertyType::Bool:
                return bool(stream.next());
            case PropertyType::Bytes:
                return stream.read_raw_string();
            case PropertyType::String:
                return stream.read_string();
            case PropertyType::VarUint:
                return QVariant::fromValue(stream.read_varuint());
            case PropertyType::Float:
                return stream.read_float();
            case PropertyType::Color:
                return QColor::fromRgba(stream.read_uint());
        }

        return {};
    }

    void preprocess_object(Object* iterator, LoadCotext& context)
    {
        if ( iterator->type_id == TypeId::Artboard )
        {
            context.artboard = context.point_to(iterator);
        }
        else if ( iterator->type_id == TypeId::KeyedObject )
        {
            if ( !context.artboard.object )
            {
                format->warning(QObject::tr("Unexpected Keyed Object"));
                return;
            }
            auto id = iterator->get<Identifier>("objectId", context.artboard.child_count);
            context.keyed_object = context.artboard_child(id);
            context.keyed_property = nullptr;
            if ( !context.keyed_object )
            {
                format->warning(QObject::tr("Invalid Keyed Object id %1").arg(id));
                return;
            }
        }
        else if ( iterator->type_id == TypeId::KeyedProperty )
        {
            if ( !context.keyed_object )
            {
                format->warning(QObject::tr("Unexpected Keyed Property"));
                return;
            }

            auto id = iterator->get<Identifier>("propertyKey");

            if ( !context.keyed_object->property_definitions.count(id) )
            {
                format->warning(QObject::tr("Unknown Keyed Property id %1").arg(id));
                return;
            }

            context.keyed_object->animations.push_back({id, {}});
            context.keyed_property = &context.keyed_object->animations.back();
        }
        else if ( iterator->has_type(TypeId::KeyFrame) )
        {
            if ( !context.keyed_property )
            {
                format->warning(QObject::tr("Unexpected Keyframe"));
                return;
            }

            context.keyed_property->keyframes.push_back(iterator);
        }
        else if ( iterator->has_value("parentId") )
        {
            auto parent_id = iterator->get<Identifier>("parentId");
            auto parent = context.artboard_child(parent_id);
            if ( !parent )
                format->warning(QObject::tr("Could not find parent with id %s").arg(parent_id));
            else
                parent->children.push_back(iterator);
        }
    }

    void process_object(Object* iterator)
    {
        if ( iterator->type_id == TypeId::Artboard )
        {
            process_artboard(iterator);
        }
    }

    void process_artboard(Object* iterator)
    {
        auto comp = document->assets()->precompositions->values.insert(std::make_unique<model::Precomposition>(document));

        comp->name.set(iterator->get<QString>("name"));
        add_shapes(iterator, comp->shapes);

        auto precomp_layer = std::make_unique<model::PreCompLayer>(document);
        precomp_layer->name.set(comp->name.get());
        precomp_layer->size.set(QSize(
            iterator->get<Float>("width"),
            iterator->get<Float>("height")
        ));
        load_transform(iterator, precomp_layer->transform.get());
        precomp_layer->opacity.set(iterator->get<Float>("opacity", 1));
        precomp_layer->composition.set(comp);
        if ( document->assets()->precompositions->values.size() == 1 )
        {
            document->main()->width.set(precomp_layer->size.get().width());
            document->main()->height.set(precomp_layer->size.get().height());
        }

        document->main()->shapes.insert(std::move(precomp_layer));
    }

    void add_shapes(Object* parent, model::ObjectListProperty<model::ShapeElement>& prop)
    {

        std::vector<std::unique_ptr<model::ShapeElement>> shapes;

        for ( Object* child : parent->children )
        {
            if ( child == parent )
            {
                format->error(QObject::tr("Parent circular reference detected"));
                continue;
            }

            auto shape = load_shape(child);
            if ( shape )
            {
                if ( child->has_type(TypeId::Path) )
                    shapes.emplace_back(std::move(shape));
                else
                    prop.insert(std::move(shape));
            }
        }

        for ( auto& shape : shapes )
            prop.insert(std::move(shape));
    }

    std::unique_ptr<model::Layer> load_shape_layer(Object* shape)
    {
        auto layer = std::make_unique<model::Layer>(document);
        load_shape_group(shape, layer.get());
        return layer;
    }

    void load_transform(Object* rive, model::Transform* transform)
    {
        transform->position.set({
            rive->get<Float>("x"),
            rive->get<Float>("y")
        });
        transform->rotation.set(rive->get<Float>("rotation"));
        transform->scale.set({
            rive->get<Float>("scaleX", 1),
            rive->get<Float>("scaleY", 1)
        });
    }

    void load_shape_group(Object* shape, model::Group* group)
    {
        group->opacity.set(shape->get<Float>("opacity", 1));
        load_transform(shape, group->transform.get());
        group->name.set(shape->get<String>("name"));
        add_shapes(shape, group->shapes);
    }

    std::unique_ptr<model::ShapeElement> load_shape(Object* shape)
    {
        switch ( shape->type_id )
        {
            case TypeId::Shape:
                return load_shape_layer(shape);
            case TypeId::Rectangle:
                return load_rectangle(shape);
            case TypeId::Fill:
                return load_fill(shape);
            case TypeId::Stroke:
                return load_stroke(shape);
            default:
                return {};
        }
    }

    std::unique_ptr<model::Group> load_rectangle(Object* obj)
    {
        auto group = std::make_unique<model::Group>(document);
        auto shape = std::make_unique<model::Rect>(document);
        shape->name.set(obj->get<String>("name"));
        shape->rounded.set((
            obj->get<Float>("cornerRadiusTL") +
            obj->get<Float>("cornerRadiusBL") +
            obj->get<Float>("cornerRadiusBR") +
            obj->get<Float>("cornerRadiusTR")
        ) / 4);
        shape->size.set({
            obj->get<Float>("width"),
            obj->get<Float>("height")
        });

        group->shapes.insert(std::move(shape));
        load_shape_group(obj, group.get());
        return group;
    }

    std::unique_ptr<model::Fill> load_fill(Object* obj)
    {
        auto shape = std::make_unique<model::Fill>(document);
        load_styler(obj, shape.get());
        /// \todo fillRule
        return shape;
    }

    std::unique_ptr<model::Stroke> load_stroke(Object* obj)
    {
        auto shape = std::make_unique<model::Stroke>(document);
        load_styler(obj, shape.get());
        shape->width.set(obj->get<Float>("thickness"));
        /// \todo cap + join
        return shape;
    }

    void load_styler(Object* object, model::Styler* shape)
    {
        shape->name.set(object->get<String>("name"));
        shape->visible.set(object->get<bool>("isVisible", true));

        for ( const auto& child : object->children )
        {
            if ( child->type_id == TypeId::SolidColor )
            {
                shape->color.set(child->get<QColor>("colorValue"));
                break;
            }
        }
    }

    model::Document* document;
    RiveStream& stream;
    RiveFormat* format;
    PropertyTable extra_props;
    bool failed = false;
};

} // namespace glaxnimate::io::rive


