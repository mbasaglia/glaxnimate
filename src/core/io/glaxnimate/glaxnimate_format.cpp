#include "glaxnimate_format.hpp"

#include <QUuid>
#include <QJsonArray>

#include "app_info.hpp"

io::Autoreg<io::glaxnimate::GlaxnimateFormat> io::glaxnimate::GlaxnimateFormat::autoreg;


bool io::glaxnimate::GlaxnimateFormat::on_save(QIODevice& file, const QString&,
                model::Document* document, const QVariantMap&)
{
    return file.write(to_json(document).toJson(QJsonDocument::Indented));
}

QJsonObject io::glaxnimate::GlaxnimateFormat::format_metadata()
{
    QJsonObject object;
    object["generator"] = AppInfo::instance().name();
    object["generator_version"] = AppInfo::instance().version();
    object["format_version"] = format_version;
    return object;
}

QJsonDocument io::glaxnimate::GlaxnimateFormat::to_json ( model::Document* document )
{
    QJsonObject doc_obj;
    doc_obj["format"] = format_metadata();
    doc_obj["metadata"] = QJsonObject::fromVariantMap(document->metadata());
    doc_obj["animation"] = to_json(document->main_composition());
    return QJsonDocument(doc_obj);
}

QJsonObject io::glaxnimate::GlaxnimateFormat::to_json ( model::Object* object )
{
    QJsonObject obj;
    obj["__type__"] = object->type_name();

    for ( model::BaseProperty* prop : object->properties() )
        obj[prop->name()] = to_json(prop);

    return obj;
}

QJsonValue io::glaxnimate::GlaxnimateFormat::to_json ( model::BaseProperty* property )
{
    if ( property->traits().flags & model::PropertyTraits::List )
    {
        QJsonArray arr;
        for ( const QVariant& val : property->value().toList() )
        {
            arr.push_back(to_json(val, property->traits()));
        }
        return arr;
    }
    else if ( property->traits().flags & model::PropertyTraits::Animated )
    {
        model::AnimatableBase* anim = static_cast<model::AnimatableBase*>(property);
        QJsonObject jso;
        if ( !anim->animated() )
        {
            jso["value"] = to_json(anim->value());
        }
        else
        {
            QJsonArray keyframes;
            for ( int i = 0, e = anim->keyframe_count(); i < e; i++ )
            {
                auto kf = anim->keyframe(i);
                QJsonObject jkf;
                jkf["time"] = kf->time();
                jkf["value"] = to_json(kf->value());
                if ( !kf->transition().hold() )
                {
                    jkf["before"] = to_json(kf->transition().before_handle());
                    jkf["after"] = to_json(kf->transition().after_handle());
                }
                keyframes.push_back(jkf);
            }
            jso["keyframes"] = keyframes;
        }
        return jso;
    }

    return to_json(property->value(), property->traits());
}

QJsonValue io::glaxnimate::GlaxnimateFormat::to_json ( const QVariant& value, model::PropertyTraits traits )
{
    if ( traits.type == model::PropertyTraits::Object )
    {
        if ( auto obj = value.value<model::Object*>() )
            return to_json(obj);
        return {};
    }
    else if ( traits.type == model::PropertyTraits::ObjectReference )
    {
        if ( auto dn = value.value<model::DocumentNode*>() )
            return QJsonValue::fromVariant(dn->uuid.get());
        return {};
    }
    else if ( traits.type == model::PropertyTraits::Enum )
    {
        return value.toString();
    }

    return to_json(value);
}


QJsonValue io::glaxnimate::GlaxnimateFormat::to_json ( const QVariant& value )
{
    if ( !value.isValid() )
        return {};


    switch ( int(value.type()) )
    {
        case QMetaType::Bool:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Long:
        case QMetaType::ULong:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Double:
        case QMetaType::Float:
        case QMetaType::QChar:
        case QMetaType::QString:
        case QMetaType::QByteArray:
        case QMetaType::QJsonArray:
        case QMetaType::QJsonObject:
        case QMetaType::QJsonValue:
        case QMetaType::QUuid:
            return QJsonValue::fromVariant(value);

        case QVariant::Invalid:
            return {};

        case QVariant::Size:
        {
            auto v = value.toSize();
            QJsonObject o;
            o["width"] = v.width();
            o["height"] = v.height();
            return o;
        }
        case QVariant::SizeF:
        {
            auto v = value.toSizeF();
            QJsonObject o;
            o["width"] = v.width();
            o["height"] = v.height();
            return o;
        }
        case QVariant::Point:
        {
            auto v = value.toPoint();
            QJsonObject o;
            o["x"] = v.x();
            o["y"] = v.y();
            return o;
        }
        case QMetaType::QVector2D:
        {
            auto v = value.value<QVector2D>();
            QJsonObject o;
            o["x"] = v.x();
            o["y"] = v.y();
            return o;
        }
        case QVariant::PointF:
        {
            auto v = value.toPointF();
            QJsonObject o;
            o["x"] = v.x();
            o["y"] = v.y();
            return o;
        }
        case QVariant::Color:
        {
            auto v = value.value<QColor>();
            QString col = v.name();
            if ( v.alpha() != 255 )
                col += QString::number(v.alpha()|0x100, 16).rightRef(2);
            return col;
        }
    }

    return {};
}
