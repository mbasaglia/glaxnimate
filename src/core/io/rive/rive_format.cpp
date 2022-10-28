#include "rive_format.hpp"

#include "rive_loader.hpp"

#include <QJsonArray>
#include <QJsonObject>

glaxnimate::io::Autoreg<glaxnimate::io::rive::RiveFormat> glaxnimate::io::rive::RiveFormat::autoreg;


bool glaxnimate::io::rive::RiveFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    BinaryInputStream stream(&file);
    if ( stream.read(4) != "RIVE" )
    {
        error(tr("Unsupported format"));
        return false;
    }

    auto vmaj = stream.read_uint_leb128();
    auto vmin = stream.read_uint_leb128();
    stream.read_uint_leb128(); // file id

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

    return RiveLoader(stream, this).load_document(document);
}

bool glaxnimate::io::rive::RiveFormat::on_save(QIODevice&, const QString&, model::Document*, const QVariantMap&)
{
    return false;
}

static QString property_type_to_string(glaxnimate::io::rive::PropertyType type)
{
    switch ( type )
    {
        case glaxnimate::io::rive::PropertyType::VarUint:
            return "VarUint";
        case glaxnimate::io::rive::PropertyType::Bool:
            return "bool";
        case glaxnimate::io::rive::PropertyType::String:
            return "string";
        case glaxnimate::io::rive::PropertyType::Bytes:
            return "bytes";
        case glaxnimate::io::rive::PropertyType::Float:
            return "float";
        case glaxnimate::io::rive::PropertyType::Color:
            return "color";
    }
    return "?";
}

QJsonDocument glaxnimate::io::rive::RiveFormat::to_json(const QByteArray& binary_data)
{
    BinaryInputStream stream(binary_data);
    if ( stream.read(4) != "RIVE" )
        return {};

    auto vmaj = stream.read_uint_leb128();
    auto vmin = stream.read_uint_leb128();
    auto file_id = stream.read_uint_leb128();

    if ( stream.has_error() || vmaj != 7 )
        return {};

    RiveLoader loader(stream, this);

    QJsonArray summary;

    QJsonArray objects;
    int id = 0;
    bool has_artboard = false;
    for ( const auto& rive_obj : loader.load_object_list() )
    {
        if ( rive_obj.type_id == TypeId::Artboard )
        {
            has_artboard = true;
            id = 0;
        }

        QJsonObject summary_obj;
        QJsonObject obj;

        QJsonArray types;
        for ( const auto& def : rive_obj.definitions )
        {
            QJsonObject jdef;
            jdef["id"] = int(def->type_id);
            jdef["name"] = def->name;
            types.push_back(jdef);
        }
        obj["class"] = types;

        QJsonArray props;
        for ( const auto& p : rive_obj.property_definitions )
        {
            QJsonObject prop;
            prop["id"] = int(p.first);
            prop["name"] = p.second.name;
            prop["type"] = property_type_to_string(p.second.type);
            auto iter = rive_obj.properties.find(p.second.name);
            QJsonValue val;

            if ( iter != rive_obj.properties.end() && iter->isValid() )
            {
                if ( iter->userType() == QMetaType::QColor )
                    val = iter->value<QColor>().name();
                else if ( iter->userType() == QMetaType::ULongLong || iter->userType() == QMetaType::ULong )
                    val = iter->toInt();
                else if ( iter->userType() == QMetaType::QByteArray )
                    val = QString::fromLatin1(iter->toByteArray().toBase64());
                else
                    val = QJsonValue::fromVariant(*iter);

                summary_obj[iter.key()] = val;
            }
            prop["value"] = val;

            props.push_back(prop);
        }
        obj["properties"] = props;

        QJsonObject summary_obj_parent;
        summary_obj_parent[rive_obj.definitions.empty() ? "?" : rive_obj.definitions[0]->name] = summary_obj;

        if ( has_artboard )
        {
            summary_obj_parent["-id"] = id;
            obj["object_id"] = id;
            id++;
        }

        objects.push_back(obj);
        summary.push_back(summary_obj_parent);
    }

    QJsonObject header;
    QJsonArray version;
    version.push_back(int(vmaj));
    version.push_back(int(vmin));
    header["version"] = version;
    header["file_id"] = int(file_id);
    QJsonArray extra_props;
    for ( const auto& p : loader.extra_properties() )
    {
        QJsonObject prop;
        prop["id"] = int(p.first);
        prop["type"] = property_type_to_string(p.second);
    }
    header["toc"] = extra_props;

    QJsonObject root;
    root["brief"] = summary;
    root["detail"] = objects;
    root["header"] = header;

    return QJsonDocument(root);
}
