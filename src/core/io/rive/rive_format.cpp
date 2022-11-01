#include "rive_format.hpp"

#include <QJsonArray>
#include <QJsonObject>


#include "rive_loader.hpp"
#include "rive_exporter.hpp"

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

    if ( vmaj != RiveFormat::format_version )
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

bool glaxnimate::io::rive::RiveFormat::on_save(QIODevice& device, const QString&, model::Document* document, const QVariantMap&)
{
    RiveExporter exporter(&device, this);
    exporter.write_document(document);
    return true;
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

    if ( stream.has_error() || vmaj != RiveFormat::format_version )
        return {};

    RiveLoader loader(stream, this);

    QJsonArray summary;

    QJsonArray objects;
    int id = 0;
    bool has_artboard = false;
    for ( const auto& rive_obj : loader.load_object_list() )
    {
        if ( !rive_obj )
        {
            summary.push_back("Invalid");
            objects.push_back("Invalid");
            continue;
        }

        if ( rive_obj.type().id == TypeId::Artboard )
        {
            has_artboard = true;
            id = 0;
        }

        QJsonObject summary_obj;
        QJsonObject obj;

        QJsonArray types;
        for ( const auto& def : rive_obj.type().definitions )
        {
            QJsonObject jdef;
            jdef["id"] = int(def->type_id);
            jdef["name"] = def->name;
            types.push_back(jdef);
        }
        obj["class"] = types;

        QJsonArray props;
        for ( const auto& p : rive_obj.type().properties )
        {
            QJsonObject prop;
            prop["id"] = int(p->id);
            prop["name"] = p->name;
            prop["type"] = property_type_to_string(p->type);
            auto iter = rive_obj.properties().find(p);
            QJsonValue val;

            if ( iter != rive_obj.properties().end() && iter->second.isValid() )
            {
                if ( iter->second.userType() == QMetaType::QColor )
                    val = iter->second.value<QColor>().name();
                else if ( iter->second.userType() == QMetaType::ULongLong || iter->second.userType() == QMetaType::ULong )
                    val = iter->second.toInt();
                else if ( iter->second.userType() == QMetaType::QByteArray )
                    val = QString::fromLatin1(iter->second.toByteArray().toBase64());
                else
                    val = QJsonValue::fromVariant(iter->second);

                summary_obj[iter->first->name] = val;
            }
            prop["value"] = val;

            props.push_back(prop);
        }
        obj["properties"] = props;

        QJsonObject summary_obj_parent;
        summary_obj_parent[!rive_obj ? "?" : rive_obj.definition()->name] = summary_obj;

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
