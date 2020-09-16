#pragma once

#include <QUuid>
#include <QJsonArray>

#include "math/bezier.hpp"
#include "glaxnimate_format.hpp"

namespace io::glaxnimate::detail {


class ImportState
{
public:
    GlaxnimateFormat* fmt;
    model::Document* document = nullptr;
    model::Composition* composition = nullptr;
    QMap<QString, model::DocumentNode*> references;
    QMap<model::BaseProperty*, QUuid> unresolved_references;
    QMap<model::Object*, QJsonObject> deferred_loads;
    std::vector<model::Object*> unwanted;

    ImportState(GlaxnimateFormat* fmt) : fmt(fmt) {}

    ~ImportState() {}

    void error(const QString& msg)
    {
        if ( fmt )
            emit fmt->error(msg);
    }

    void resolve()
    {
        for ( auto it = unresolved_references.begin(); it != unresolved_references.end(); ++it )
        {
            model::BaseProperty* prop = it.key();
            model::DocumentNode* node = document->find_by_uuid(*it);
            if ( !node )
            {
                error(GlaxnimateFormat::tr("Property %1 of %2 refers to unexisting object %3")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                    .arg(it->toString())
                );
            }
            else
            {
                if ( !prop->set_value(QVariant::fromValue(node)) )
                    error(GlaxnimateFormat::tr("Could not load %1 for %2: uuid refers to an unacceptable object")
                        .arg(prop->name())
                        .arg(prop->object()->object_name())
                    );
            }
        }

        for ( model::Object* obj : unwanted )
        {
            if ( obj )
            {
                error(GlaxnimateFormat::tr("Object %1 is invalid").arg(obj->object_name()));
                delete obj;
            }
        }
    }

    void load_object ( model::Object* target, QJsonObject object )
    {
        QString type = object["__type__"].toString();

        if ( type != target->type_name() )
            error(GlaxnimateFormat::tr("Wrong object type: expected '%1' but got '%2'").arg(target->type_name()).arg(type));


        if ( auto node = qobject_cast<model::DocumentNode*>(target) )
        {
            node->docnode_set_visible(object["visible"].toBool(true));
            object.remove("visible");
            node->docnode_set_locked(object["locked"].toBool(false));
            object.remove("locked");
        }

        for ( model::BaseProperty* prop : target->properties() )
        {
            if ( !load_prop(prop, object[prop->name()]) )
                error(GlaxnimateFormat::tr("Could not load %1 for %2")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                );
        }

        for ( auto it = object.begin(); it != object.end(); ++it )
        {
            if ( !target->has(it.key()) && it.key() != "__type__" )
            {
                if ( !target->set(it.key(), it->toVariant()) )
                    error(GlaxnimateFormat::tr("Could not set property %1").arg(it.key()));
            }
        }
    }

    bool load_prop ( model::BaseProperty* target, const QJsonValue& val )
    {
        if ( target->traits().flags & model::PropertyTraits::List )
        {
            if ( !val.isArray() )
                return false;

            QVariantList list;
            for ( const QJsonValue& item : val.toArray() )
                list.push_back(load_prop_value(target, item, false));

            if ( target->traits().type == model::PropertyTraits::Object )
            {
                int index = 0;
                for ( const QVariant& item : list )
                {
                    auto ptr = item.value<model::Object*>();
                    model::ObjectListPropertyBase* prop = static_cast<model::ObjectListPropertyBase*>(target);
                    if ( !ptr )
                    {
                        error(
                            GlaxnimateFormat::tr("Item %1 for %2 in %3 isn't an object")
                            .arg(index)
                            .arg(target->name())
                            .arg(target->object()->object_name())
                        );
                    }
                    else
                    {
                        auto inserted = prop->insert_clone(ptr);
                        if ( !inserted )
                        {
                            error(
                                GlaxnimateFormat::tr("Item %1 for %2 in %3 is not acceptable")
                                .arg(index)
                                .arg(target->name())
                                .arg(target->object()->object_name())
                            );
                        }
                        else
                        {
                            load_object(inserted, deferred_loads[ptr]);
                        }
                        deferred_loads.remove(ptr);
                    }
                    index++;
                }

                return true;
            }
            else
            {
                return target->set_value(list);
            }
        }
        else if ( target->traits().flags & model::PropertyTraits::Animated )
        {
            QJsonObject jso = val.toObject();
            if ( jso.contains("value") )
            {
                return target->set_value(load_prop_value(target, jso["value"], true));
            }
            else
            {
                model::AnimatableBase* anim = static_cast<model::AnimatableBase*>(target);
                for ( const auto& v : jso["keyframes"].toArray() )
                {
                    QJsonObject kfobj = v.toObject();
                    if ( !kfobj.contains("time") )
                    {
                        error(GlaxnimateFormat::tr("Keyframe must specify a time"));
                        continue;
                    }
                    if ( !kfobj.contains("value") )
                    {
                        error(GlaxnimateFormat::tr("Keyframe must specify a value"));
                        continue;
                    }

                    model::KeyframeBase* kf = anim->set_keyframe(
                        kfobj["time"].toDouble(),
                        load_prop_value(target, kfobj["value"], false)
                    );
                    if ( !kf )
                    {
                        error(GlaxnimateFormat::tr("Could not add keyframe"));
                        continue;
                    }

                    QPointF before, after;
                    if ( load_2d(kfobj["before"], "x", "y", before) && load_2d(kfobj["after"], "x", "y", after) )
                    {
                        kf->transition().set_before_handle(before);
                        kf->transition().set_after_handle(after);
                    }
                    else
                    {
                        kf->transition().set_hold(true);
                    }
                }
            }
        }


        if ( target->traits().type == model::PropertyTraits::ObjectReference )
        {
            QUuid uuid(val.toString());
            if ( !uuid.isNull() )
                unresolved_references[target] = uuid;
            return true;
        }
        else if ( target->traits().type == model::PropertyTraits::Uuid )
        {
            QUuid uuid(val.toString());
            if ( uuid.isNull() )
                return false;
            return target->set_value(uuid);
        }

        QVariant loaded_val = load_prop_value(target, val, true);
        if ( !target->set_value(loaded_val) )
        {
            if ( target->traits().type == model::PropertyTraits::Object )
                unwanted.push_back(loaded_val.value<model::Object*>());
            return false;
        }
        return true;
    }


    QVariant load_prop_value ( model::BaseProperty* target, const QJsonValue& val, bool load_objects )
    {
        switch ( target->traits().type )
        {
            case model::PropertyTraits::Object:
            {
                if ( !val.isObject() )
                    return {};
                QJsonObject jobj = val.toObject();
                model::Object* object = create_object(jobj["__type__"].toString());
                if ( !object )
                    return {};
                if ( load_objects )
                    load_object(object, jobj);
                else
                    deferred_loads.insert(object, jobj);
                return QVariant::fromValue(object);
            }
            case model::PropertyTraits::ObjectReference:
            case model::PropertyTraits::Uuid:
                // handled above
                return {};
            case model::PropertyTraits::Color:
            {
                QString name = val.toString();
                // We want #rrggbbaa, qt does #aarrggbb
                if ( name.startsWith("#") && name.size() == 9 )
                {
                    int alpha = name.right(2).toInt(nullptr, 16);
                    QColor col(name);
                    col.setAlpha(alpha);
                    return col;
                }
                return QColor(name);

            }
            case model::PropertyTraits::Point:
            {
                QPointF p;
                if ( load_2d(val, "x", "y", p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Size:
            {
                QSizeF p;
                if ( load_2d(val, "width", "height", p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Scale:
            {
                QVector2D p;
                if ( load_2d(val, "x", "y", p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Bezier:
            {
                if ( !val.isObject() )
                    return {};

                math::Bezier bezier;
                QJsonObject obj = val.toObject();
                bezier.set_closed(obj["closed"].toBool());

                for ( const auto& jspv : obj["points"].toArray() )
                {
                    if ( !jspv.isObject() )
                        continue;
                    QJsonObject jsp = jspv.toObject();
                    math::BezierPoint p{{}, {}, {}};
                    load_2d(jsp["pos"], "x", "y", p.pos);
                    load_2d(jsp["tan_in"], "x", "y", p.tan_in);
                    load_2d(jsp["tan_out"], "x", "y", p.tan_out);
                    p.type = math::BezierPointType(jsp["type"].toInt());
                    bezier.push_back(p);
                }
                return QVariant::fromValue(bezier);
            }
            default:
                return val.toVariant();
        }
    }

    template<class Type>
    bool load_2d(const QJsonValue& val, const QString& x, const QString& y, Type& ret)
    {
        QJsonObject obj = val.toObject();
        if ( obj.empty() )
            return false;
        ret = Type(obj[x].toDouble(), obj[y].toDouble());
        return true;
    }

    model::Object* create_object(const QString& type)
    {
        if ( type == "MainComposition" )
        {
            error(GlaxnimateFormat::tr("Objects of type 'MainComposition' can only be at the top level of the document"));
            return nullptr;
        }

        if ( auto obj = model::Factory::instance().make_any(type, document, composition) )
            return obj;

        error(GlaxnimateFormat::tr("Unknow object of type '%1'").arg(type));
        return new model::Object(document);
    }

};

} // namespace io::glaxnimate::detail
