/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QUuid>
#include <QJsonArray>

#include "math/bezier/bezier.hpp"
#include "glaxnimate_format.hpp"
#include "model/assets/assets.hpp"

namespace glaxnimate::io::glaxnimate::detail {

class ImportState
{
private:
    struct UnresolvedPath
    {
        struct Step
        {
            model::Object* object = nullptr;
            model::BaseProperty* prop = nullptr;
        };

        struct Item
        {
            QString propname;
            int index = -1;

            model::Object* step(model::Object* prev) const
            {
                auto prop = prev->get_property(propname);
                if ( !prop || prop->traits().type != model::PropertyTraits::Object )
                    return nullptr;

                if ( prop->traits().flags & model::PropertyTraits::List )
                {
                    if ( index == -1 )
                        return nullptr;

                    auto val_list = prop->value().toList();
                    if ( val_list.size() <= index )
                        return nullptr;

                    return val_list[index].value<model::Object*>();
                }

                return prop->value().value<model::Object*>();
            }
        };

        UnresolvedPath(model::Object* base_object = nullptr) : base_object(base_object)
        {}

        UnresolvedPath sub(model::BaseProperty* prop) const
        {
            auto copy = *this;
            copy.items.push_back({prop->name()});
            return copy;
        }

        UnresolvedPath sub(int index) const
        {
            auto copy = *this;
            copy.items.back().index = index;
            return copy;
        }

        model::BaseProperty* prop() const
        {
            if ( items.empty() || !base_object )
                return nullptr;

            model::Object*  object = base_object;
            for ( int i = 0, e = items.size() - 1; i < e; i++ )
            {
                object = items[i].step(object);
                if ( !object )
                    return nullptr;
            }

            return object->get_property(items.back().propname);
        }

        model::Object* base_object = nullptr;
        std::vector<Item> items;
    };

public:
    ImportState(GlaxnimateFormat* fmt, model::Document* document, int document_version = GlaxnimateFormat::format_version)
    : fmt(fmt), document(document), document_version(document_version)
    {}

    ~ImportState() {}

    void resolve()
    {
        for ( const auto& p : unresolved_references )
        {
            model::BaseProperty* prop = p.first.prop();
            model::DocumentNode* node = document->find_by_uuid(p.second);
            if ( !node )
            {
                error(GlaxnimateFormat::tr("Property %1 of %2 refers to unexisting object %3")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                    .arg(p.second.toString())
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
        version_fixup(object);
        do_load_object(target, object, target);
    }

    void load_metadata(const QJsonObject& object)
    {
        document->metadata() = object["metadata"_qs].toObject().toVariantMap();
        auto info = object["info"_qs];
        document->info().author = info["author"_qs].toString();
        document->info().description = info["description"_qs].toString();
        for ( const auto& kw: info["keywords"_qs].toArray() )
            document->info().keywords.push_back(kw.toString());
    }

    void load_document(QJsonObject top_level)
    {
        auto assets = top_level[document_version < 3 ? "defs"_qs : "assets"_qs].toObject();

        if ( document_version < 8 )
        {
            QJsonObject precomps;
            QJsonArray values;
            if ( assets.contains("precompositions"_qs) )
            {
                precomps = assets["precompositions"_qs].toObject();
                values = precomps["values"_qs].toArray();
            }
            else
            {
                precomps["__type__"_qs] = "CompositionList"_qs;
            }

            if ( top_level["animation"_qs].isObject() )
            {
                QJsonObject main = top_level["animation"_qs].toObject();
                top_level.remove("animation"_qs);
                values.push_front(main);
            }

            precomps["values"_qs] = values;
            assets["precompositions"_qs] = precomps;
        }

        load_metadata(top_level);
        load_object(document->assets(), assets);
        resolve();
    }

private:
    void error(const QString& msg)
    {
        if ( fmt )
            Q_EMIT fmt->warning(msg);
    }

    QJsonObject fixed_asset_list(const QString& type, const QJsonValue& values)
    {
        QJsonObject fixed;
        fixed["__type__"_qs] = type;
        fixed["values"_qs] = values;
        fixed["uuid"_qs] = QUuid::createUuid().toString();
        return fixed;
    }

    void version_fixup(QJsonObject& object)
    {
        if ( document_version == 1 )
        {
            QString type = object["__type__"_qs].toString();
            static const auto fix_ac = [](QJsonObject& object){
                QJsonObject ac;
                ac["__type__"_qs] = "AnimationContainer"_qs;
                ac["first_frame"_qs] = object["first_frame"_qs];
                ac["last_frame"_qs] = object["last_frame"_qs];
                object.remove("first_frame"_qs);
                object.remove("last_frame"_qs);
            };

            if ( type == "MainComposition"_qs )
            {
                fix_ac(object);
                object["shapes"_qs] = object["layers"_qs];
                object.remove("layers"_qs);
            }
            else if ( type == "ShapeLayer"_qs )
            {
                fix_ac(object);
                object["__type__"_qs] = "Layer"_qs;
            }
            else if ( type == "EmptyLayer"_qs )
            {
                fix_ac(object);
                object["__type__"_qs] = "Layer"_qs;
                object["shapes"_qs] = QJsonArray();
            }
        }

        if ( document_version < 3 && object["__type__"_qs].toString() == "Defs"_qs )
        {
            static const std::vector<std::pair<QString, QString>> types = {
                {"colors"_qs, "NamedColorList"_qs},
                {"gradient_colors"_qs, "GradientColorsList"_qs},
                {"gradients"_qs, "GradientList"_qs},
                {"images"_qs, "BitmapList"_qs},
                {"precompositions"_qs, "PrecompositionList"_qs},
            };

            for ( const auto & pair : types )
            {
                if ( object.contains(pair.first) )
                {
                    object[pair.first] = fixed_asset_list(pair.second, object[pair.first]);
                }
            }

            object["uuid"_qs] = QUuid::createUuid().toString();
            object["__type__"_qs] = "Assets"_qs;
        }

        if ( document_version < 4 && object["__type__"_qs].toString() == "Assets"_qs )
        {
            object["fonts"_qs] = fixed_asset_list("FontList"_qs, QJsonArray());
        }

        if ( document_version < 5 )
        {
            if ( object["__type__"_qs].toString() == "Trim"_qs )
            {
                // values were swapped
                if ( object["multiple"_qs].toString() == "Individually"_qs )
                    object["multiple"_qs] = "Simultaneously"_qs;
                else
                    object["multiple"_qs] = "Individually"_qs;
            }
        }

        if ( document_version < 6 )
        {
            if ( object["__type__"_qs].toString() == "MaskSettings"_qs )
                object["mask"_qs] = int(object["mask"_qs].toBool());
        }


        if ( document_version < 8 )
        {
            if ( object["__type__"_qs].toString() == "MainComposition"_qs )
            {
                object["__type__"_qs] = "Composition"_qs;
            }
            else if ( object["__type__"_qs].toString() == "Precomposition"_qs )
            {
                object["__type__"_qs] = "Composition"_qs;
                if ( !document->assets()->compositions->values.empty() )
                {
                    auto main = document->assets()->compositions->values[0];
                    if ( !object.contains("fps"_qs) )
                        object["fps"_qs] = main->fps.get();
                    if ( !object.contains("width"_qs) )
                        object["width"_qs] = main->width.get();
                    if ( !object.contains("height"_qs) )
                        object["height"_qs] = main->height.get();
                }
            }
            else if ( object["__type__"_qs].toString() == "PrecompositionList"_qs )
            {
                object["__type__"_qs] = "CompositionList"_qs;
            }
            else if ( object["__type__"_qs].toString() == "Assets"_qs )
            {
                QJsonObject comps = object["precompositions"_qs].toObject();
                object.remove("precompositions"_qs);
                object["compositions"_qs] = comps;
            }
        }
    }

    void do_load_object ( model::Object* target, QJsonObject object, const UnresolvedPath& path )
    {
        QString type = object["__type__"_qs].toString();

        if ( type != target->type_name() )
            error(GlaxnimateFormat::tr("Wrong object type: expected '%1' but got '%2'").arg(target->type_name()).arg(type));

        for ( model::BaseProperty* prop : target->properties() )
        {
            if ( object.contains(prop->name()) && !load_prop(prop, object[prop->name()], path.sub(prop)) )
            {
                error(GlaxnimateFormat::tr("Could not load %1 for %2")
                    .arg(prop->name())
                    .arg(prop->object()->object_name())
                );
            }
        }

        for ( auto it = object.begin(); it != object.end(); ++it )
        {
            if ( !target->has(it.key()) && it.key() != "__type__"_qs )
            {
                if ( !target->set(it.key(), it->toVariant()) )
                    error(GlaxnimateFormat::tr("Could not set property %1").arg(it.key()));
            }
        }
    }

    bool load_prop ( model::BaseProperty* target, const QJsonValue& val, const UnresolvedPath& path )
    {
        if ( target->traits().flags & model::PropertyTraits::List )
        {
            if ( !val.isArray() )
                return false;

            QVariantList list;
            for ( QJsonValue item : val.toArray() )
                list.push_back(load_prop_value(target, item, false, {}));

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
                            do_load_object(inserted, deferred_loads[ptr], path.sub(index));
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
            if ( jso.contains("value"_qs) )
            {
                return target->set_value(load_prop_value(target, jso["value"_qs], true, path));
            }
            else
            {
                model::AnimatableBase* anim = static_cast<model::AnimatableBase*>(target);
                bool position = anim->traits().type == model::PropertyTraits::Point;

                for ( auto v : jso["keyframes"_qs].toArray() )
                {
                    QJsonObject kfobj = v.toObject();
                    if ( !kfobj.contains("time"_qs) )
                    {
                        error(GlaxnimateFormat::tr("Keyframe must specify a time"));
                        continue;
                    }
                    if ( !kfobj.contains("value"_qs) )
                    {
                        error(GlaxnimateFormat::tr("Keyframe must specify a value"));
                        continue;
                    }

                    model::KeyframeBase* kf = anim->set_keyframe(
                        kfobj["time"_qs].toDouble(),
                        load_prop_value(target, kfobj["value"_qs], false, {})
                    );
                    if ( !kf )
                    {
                        error(GlaxnimateFormat::tr("Could not add keyframe"));
                        continue;
                    }

                    QPointF before, after;
                    if ( load_2d(kfobj["before"_qs], "x"_qs, "y"_qs, before) && load_2d(kfobj["after"_qs], "x"_qs, "y"_qs, after) )
                    {
                        kf->set_transition({before, after});
                    }
                    else
                    {
                        kf->set_transition({{0, 0}, {1, 1}, true});
                    }

                    if ( position )
                    {
                        auto pkf = static_cast<model::Keyframe<QPointF>*>(kf);
                        QPointF tan_in = pkf->get();
                        QPointF tan_out = pkf->get();
                        bool load_ti = load_2d(kfobj["tan_in"_qs], "x"_qs, "y"_qs, tan_in);
                        bool load_to = load_2d(kfobj["tan_out"_qs], "x"_qs, "y"_qs, tan_out);
                        if ( load_ti || load_to )
                        {
                            auto type = math::bezier::PointType(kfobj["point_type"_qs].toInt());
                            pkf->set_point({pkf->get(), tan_in, tan_out, type});
                        }
                    }
                }

                return true;
            }
        }


        if ( target->traits().type == model::PropertyTraits::ObjectReference )
        {
            QUuid uuid(val.toString());
            if ( !uuid.isNull() )
                unresolved_references.emplace_back(path, uuid);
            return true;
        }
        else if ( target->traits().type == model::PropertyTraits::Uuid )
        {
            QUuid uuid(val.toString());
            if ( uuid.isNull() )
                return false;
            return target->set_value(uuid);
        }

        QVariant loaded_val = load_prop_value(target, val, true, path);
        if ( !target->set_value(loaded_val) )
        {
            if ( target->traits().type == model::PropertyTraits::Object )
                unwanted.push_back(loaded_val.value<model::Object*>());
            return false;
        }
        return true;
    }

    QColor load_color(const QJsonValue& val)
    {
        QString name = val.toString();
        // We want #rrggbbaa, qt does #aarrggbb
        if ( name.startsWith("#"_qs) && name.size() == 9 )
        {
            int alpha = name.right(2).toInt(nullptr, 16);
            QColor col(name.left(7));
            col.setAlpha(alpha);
            return col;
        }
        return QColor(name);
    }


    QVariant load_prop_value ( model::BaseProperty* target, const QJsonValue& val, bool load_objects, const UnresolvedPath& path )
    {
        switch ( target->traits().type )
        {
            case model::PropertyTraits::Object:
            {
                if ( !val.isObject() )
                    return {};
                QJsonObject jobj = val.toObject();
                version_fixup(jobj);
                model::Object* object = create_object(jobj["__type__"_qs].toString());
                if ( !object )
                    return {};
                if ( load_objects )
                    do_load_object(object, jobj, path);
                else
                    deferred_loads.insert(object, jobj);
                return QVariant::fromValue(object);
            }
            case model::PropertyTraits::ObjectReference:
            case model::PropertyTraits::Uuid:
                // handled above
                return {};
            case model::PropertyTraits::Color:
                return load_color(val);
            case model::PropertyTraits::Point:
            {
                QPointF p;
                if ( load_2d(val, "x"_qs, "y"_qs, p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Size:
            {
                QSizeF p;
                if ( load_2d(val, "width"_qs, "height"_qs, p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Scale:
            {
                QVector2D p;
                if ( load_2d(val, "x"_qs, "y"_qs, p) )
                    return p;
                return {};
            }
            case model::PropertyTraits::Bezier:
            {
                if ( !val.isObject() )
                    return {};

                math::bezier::Bezier bezier;
                QJsonObject obj = val.toObject();
                bezier.set_closed(obj["closed"_qs].toBool());

                for ( auto jspv : obj["points"_qs].toArray() )
                {
                    if ( !jspv.isObject() )
                        continue;
                    QJsonObject jsp = jspv.toObject();
                    math::bezier::Point p{{}, {}, {}};
                    load_2d(jsp["pos"_qs], "x"_qs, "y"_qs, p.pos);
                    load_2d(jsp["tan_in"_qs], "x"_qs, "y"_qs, p.tan_in);
                    load_2d(jsp["tan_out"_qs], "x"_qs, "y"_qs, p.tan_out);
                    p.type = math::bezier::PointType(jsp["type"_qs].toInt());
                    bezier.push_back(p);
                }
                return QVariant::fromValue(bezier);
            }
            case model::PropertyTraits::Gradient:
            {
                if ( !val.isArray() )
                    return {};

                QGradientStops stops;
                for ( auto jstopv : val.toArray() )
                {
                    if ( !jstopv.isObject() )
                        continue;
                    auto jstop = jstopv.toObject();
                    stops.push_back({jstop["offset"_qs].toDouble(), load_color(jstop["color"_qs])});
                }

                return QVariant::fromValue(stops);
            }
            case model::PropertyTraits::Data:
                return QByteArray::fromBase64(val.toString().toLatin1());
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
        if ( auto obj = model::Factory::instance().build(type, document) )
        {
            temporaries.emplace_back(obj);
            return obj;
        }

        error(GlaxnimateFormat::tr("Unknown object of type '%1'").arg(type));
        temporaries.emplace_back(new model::Object(document));
        return temporaries.back().get();
    }


    GlaxnimateFormat* fmt;
    model::Document* document = nullptr;
    QMap<QString, model::DocumentNode*> references;
    std::vector<std::pair<UnresolvedPath, QUuid>> unresolved_references;
    QMap<model::Object*, QJsonObject> deferred_loads;
    std::vector<model::Object*> unwanted;
    std::vector<std::unique_ptr<model::Object>> temporaries;
    int document_version;
};

} // namespace glaxnimate::io::glaxnimate::detail
