/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_format.hpp"

#include <QUuid>
#include <QJsonArray>

#include "app_info.hpp"
#include "math/bezier/bezier.hpp"
#include "model/assets/assets.hpp"
#include "app/utils/string_view.hpp"

using namespace glaxnimate;

io::Autoreg<io::glaxnimate::GlaxnimateFormat> io::glaxnimate::GlaxnimateFormat::autoreg;

const int glaxnimate::io::glaxnimate::GlaxnimateFormat::format_version = 8;



bool io::glaxnimate::GlaxnimateFormat::on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&)
{
    return file.write(to_json(comp->document()).toJson(QJsonDocument::Indented));
}

QJsonObject io::glaxnimate::GlaxnimateFormat::format_metadata()
{
    QJsonObject object;
    object["generator"_qs] = AppInfo::instance().name();
    object["generator_version"_qs] = AppInfo::instance().version();
    object["format_version"_qs] = format_version;
    return object;
}

QJsonDocument io::glaxnimate::GlaxnimateFormat::to_json ( model::Document* document )
{
    QJsonObject doc_obj;
    doc_obj["format"_qs] = format_metadata();
    doc_obj["metadata"_qs] = QJsonObject::fromVariantMap(document->metadata());
    QJsonObject info;
    info["author"_qs] = document->info().author;
    info["description"_qs] = document->info().description;
    QJsonArray keywords;
    for ( const auto& kw: document->info().keywords )
        keywords.push_back(kw);
    info["keywords"_qs] = keywords;
    doc_obj["info"_qs] = info;
    doc_obj["assets"_qs] = to_json(document->assets());
    return QJsonDocument(doc_obj);
}

QJsonObject io::glaxnimate::GlaxnimateFormat::to_json ( model::Object* object )
{
    QJsonObject obj;
    obj["__type__"_qs] = object->type_name();

    for ( model::BaseProperty* prop : object->properties() )
        obj[prop->name()] = to_json(prop);

    return obj;
}

namespace  {

QJsonValue point_to_json(const QPointF& v)
{
    QJsonObject o;
    o["x"_qs] = v.x();
    o["y"_qs] = v.y();
    return o;
}

} // namespace


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
        bool position = anim->traits().type == model::PropertyTraits::Point;
        QJsonObject jso;
        if ( !anim->animated() )
        {
            jso["value"_qs] = to_json(anim->value(), property->traits());
        }
        else
        {
            QJsonArray keyframes;
            for ( int i = 0, e = anim->keyframe_count(); i < e; i++ )
            {
                auto kf = anim->keyframe(i);
                QJsonObject jkf;
                jkf["time"_qs] = kf->time();
                jkf["value"_qs] = to_json(kf->value(), property->traits());
                if ( !kf->transition().hold() )
                {
                    jkf["before"_qs] = to_json(kf->transition().before());
                    jkf["after"_qs] = to_json(kf->transition().after());
                }

                if ( position )
                {
                    auto pkf = static_cast<model::Keyframe<QPointF>*>(kf);
                    jkf["tan_in"_qs] = point_to_json(pkf->point().tan_in);
                    jkf["tan_out"_qs] = point_to_json(pkf->point().tan_out);
                    jkf["point_type"_qs] = pkf->point().type;
                }

                keyframes.push_back(jkf);
            }
            jso["keyframes"_qs] = keyframes;
        }
        return jso;
    }

    return to_json(property->value(), property->traits());
}

QJsonValue io::glaxnimate::GlaxnimateFormat::to_json ( const QVariant& value, model::PropertyTraits traits )
{
    switch ( traits.type )
    {
        case model::PropertyTraits::Object:
            if ( auto obj = value.value<model::Object*>() )
                return to_json(obj);
            return {};
        case model::PropertyTraits::ObjectReference:
            if ( auto dn = value.value<model::DocumentNode*>() )
                return QJsonValue::fromVariant(dn->uuid.get());
            return {};
        case model::PropertyTraits::Enum:
            return value.toString();
        case model::PropertyTraits::Bezier:
        {
            math::bezier::Bezier bezier = value.value<math::bezier::Bezier>();
            QJsonObject jsbez;
            jsbez["closed"_qs] = bezier.closed();
            QJsonArray points;
            for ( const auto& p : bezier )
            {
                QJsonObject jsp;
                jsp["pos"_qs] = point_to_json(p.pos);
                jsp["tan_in"_qs] = point_to_json(p.tan_in);
                jsp["tan_out"_qs] = point_to_json(p.tan_out);
                jsp["type"_qs] = p.type;
                points.push_back(jsp);
            }
            jsbez["points"_qs] = points;
            return jsbez;
        }
        case model::PropertyTraits::Gradient:
        {
            QJsonArray stops;
            for ( const auto& stop : value.value<QGradientStops>() )
            {
                QJsonObject jstop;
                jstop["offset"_qs] = stop.first;
                jstop["color"_qs] = to_json(stop.second);
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


    switch ( value.userType() )
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
            return QString::fromLatin1(value.toByteArray().toBase64());

        case QMetaType::UnknownType:
            return {};

        case QMetaType::QSize:
        {
            auto v = value.toSize();
            QJsonObject o;
            o["width"_qs] = v.width();
            o["height"_qs] = v.height();
            return o;
        }
        case QMetaType::QSizeF:
        {
            auto v = value.toSizeF();
            QJsonObject o;
            o["width"_qs] = v.width();
            o["height"_qs] = v.height();
            return o;
        }
        case QMetaType::QPoint:
        {
            auto v = value.toPoint();
            QJsonObject o;
            o["x"_qs] = v.x();
            o["y"_qs] = v.y();
            return o;
        }
        case QMetaType::QVector2D:
        {
            auto v = value.value<QVector2D>();
            QJsonObject o;
            o["x"_qs] = v.x();
            o["y"_qs] = v.y();
            return o;
        }
        case QMetaType::QPointF:
            return point_to_json(value.toPointF());
        case QMetaType::QColor:
        {
            auto v = value.value<QColor>();
            QString col = v.name();
            if ( v.alpha() != 255 )
                col += ::utils::right_ref(QString::number(v.alpha()|0x100, 16), 2);
            return col;
        }
    }

    if ( value.canConvert<QPointF>() )
        return point_to_json(value.toPointF());

    return {};
}
