#include "rive_format.hpp"

#include "rive_stream.hpp"
#include "type_def.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::rive::RiveFormat> glaxnimate::io::rive::RiveFormat::autoreg;

namespace glaxnimate::io::rive {

class RiveLoader
{
public:
    RiveLoader(int vmaj, int vmin, model::Document* document, RiveStream& stream, RiveFormat* format)
        : document(document),
        stream(stream),
        format(format)
    {
        Q_UNUSED(vmaj);
        Q_UNUSED(vmin);
    }

    bool load_document()
    {
        extra_props = stream.read_property_table();

        if ( stream.has_error() )
        {
            format->error(QObject::tr("Could not read property table"));
            return false;
        }

        std::vector<Object> objects;

        while ( !stream.has_error() && !stream.eof() )
            objects.emplace_back(read_object());

        return true;
    }

    bool gather_definitions(Identifier type_id, Object& object)
    {
        auto it = defined_objects.find(type_id);
        if ( it == defined_objects.end() )
        {
            fail(QObject::tr("Unknown object of type %1").arg(type_id));
            return {};
        }

        const auto& def = it->second;

        object.definitions.push_back(&def);

        if ( def.extends )
        {
            if ( !gather_definitions(def.extends, object) )
                return false;
        }

        object.property_definitions.insert(def.properties.begin(), def.properties.end());
        return true;
    }

    Object read_object()
    {
        auto type_id = stream.read_varuint();
        if ( stream.has_error() )
        {
            fail(QObject::tr("Could not load object type ID"));
            return {};
        }

        Object obj;
        obj.definition_id = type_id;

        if ( !gather_definitions(type_id, obj) )
            return {};

        while ( true )
        {
            Identifier prop_id = stream.read_varuint();
            if ( stream.has_error() )
            {
                fail(QObject::tr("Could not load property ID in %1 (%2)").arg(type_id).arg(obj.definitions[0]->name));
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
                    fail(QObject::tr("Unknown property %1 of %2 (%3)").arg(prop_id).arg(type_id).arg(obj.definitions[0]->name));
                    return {};
                }
                else
                {
                    format->warning(QObject::tr("Skipping unknown property %1 of %2 (%3)").arg(prop_id).arg(type_id).arg(obj.definitions[0]->name));
                }
            }
            else
            {
                obj.properties[prop_def->second.name] = read_property_value(prop_def->second.type);
                if ( stream.has_error() )
                {
                    fail(QObject::tr("Error loading property %1 (%2) of %3 (%4)").arg(prop_id).arg(prop_def->second.name).arg(type_id).arg(obj.definitions[0]->name));
                    return {};
                }
            }
        }

        qDebug() << obj.definitions[0]->name << obj.definition_id << obj.properties;
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

    model::Document* document;
    RiveStream& stream;
    RiveFormat* format;
    PropertyTable extra_props;
    bool failed = false;
};

} // namespace glaxnimate::io::rive


bool glaxnimate::io::rive::RiveFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    RiveStream stream(&file);
    if ( stream.read(4) != "RIVE" )
    {
        error(tr("Unsupported format"));
        return false;
    }

    auto vmaj = stream.read_varuint();
    auto vmin = stream.read_varuint();
    stream.read_varuint(); // file id

    if ( stream.has_error() )
    {
        error(tr("Could not read header"));
        return false;
    }

    if ( vmaj != 7 )
    {
        error(tr("Loading unsupported rive file version %1.%2, the only supported version is %3").arg(vmaj).arg(vmin).arg(7));
        return false;
    }

    if ( stream.has_error() )
    {
        error(tr("Could not read property table"));
        return false;
    }

    return RiveLoader(vmaj, vmin, document, stream, this).load_document();
}

bool glaxnimate::io::rive::RiveFormat::on_save(QIODevice&, const QString&, model::Document*, const QVariantMap&)
{
    return false;
}


