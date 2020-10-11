#include "glaxnimate_format.hpp"

#include <QUuid>
#include <QJsonArray>

#include "app_info.hpp"
#include "math/bezier.hpp"
#include "model/defs/defs.hpp"

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
    doc_obj["defs"] = to_json(document->defs());
    doc_obj["animation"] = to_json(document->main());
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
            jso["value"] = to_json(anim->value(), property->traits());
        }
        else
        {
            QJsonArray keyframes;
            for ( int i = 0, e = anim->keyframe_count(); i < e; i++ )
            {
                auto kf = anim->keyframe(i);
                QJsonObject jkf;
                jkf["time"] = kf->time();
                jkf["value"] = to_json(kf->value(), property->traits());
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

namespace detail {

QJsonValue to_json(const QPointF& v)
{
    QJsonObject o;
    o["x"] = v.x();
    o["y"] = v.y();
    return o;
}

} // namespace detail

QJsonValue io::glaxnimate::GlaxnimateFormat::to_json ( const QVariant& value, model::PropertyTraits traits )
{
    switch ( traits.type )
    {
        case model::PropertyTraits::Object:
            if ( auto obj = value.value<model::Object*>() )
                return to_json(obj);
            return {};
        case model::PropertyTraits::ObjectReference:
            if ( auto dn = value.value<model::ReferenceTarget*>() )
                return QJsonValue::fromVariant(dn->uuid.get());
            return {};
        case model::PropertyTraits::Enum:
            return value.toString();
        case model::PropertyTraits::Bezier:
        {
            math::Bezier bezier = value.value<math::Bezier>();
            QJsonObject jsbez;
            jsbez["closed"] = bezier.closed();
            QJsonArray points;
            for ( const auto& p : bezier )
            {
                QJsonObject jsp;
                jsp["pos"] = detail::to_json(p.pos);
                jsp["tan_in"] = detail::to_json(p.tan_in);
                jsp["tan_out"] = detail::to_json(p.tan_out);
                jsp["type"] = p.type;
                points.push_back(jsp);
            }
            jsbez["points"] = points;
            return jsbez;
        }
        case model::PropertyTraits::Gradient:
        {
            QJsonArray stops;
            for ( const auto stop : value.value<QGradientStops>() )
            {
                QJsonObject jstop;
                jstop["offset"] = stop.first;
                jstop["color"] = to_json(stop.second);
                stops.push_back(jstop);
            }
            return stops;
        }
        default:
            return to_json(value);
    }
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
        case QMetaType::QJsonArray:
        case QMetaType::QJsonObject:
        case QMetaType::QJsonValue:
        case QMetaType::QUuid:
            return QJsonValue::fromVariant(value);

        case QMetaType::QByteArray:
            return QString(value.toByteArray().toBase64());

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
            return detail::to_json(value.toPointF());
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
