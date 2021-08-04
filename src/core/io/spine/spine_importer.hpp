#pragma once

#include <QJsonObject>
#include <QJsonArray>

#include "model/shapes/layer.hpp"
#include "model/skeleton/skeleton.hpp"
#include "spine_format.hpp"

namespace glaxnimate::io::spine {


class SpineImporter
{
public:
    SpineImporter(model::Document* document, io::ImportExport* ie)
        : document(document),
        ie(ie)
    {}

    void load_document(const QJsonObject& json)
    {
        QJsonObject skeleton = json["skeleton"].toObject();
        document->main()->fps.set(get(skeleton, "fps", 30));
        auto layer = std::make_unique<model::Layer>(document);
        // width/height from the file for some reason don't match
        /*
        document->main()->width.set(get(skeleton, "width", document->main()->width.get()));
        document->main()->height.set(get(skeleton, "height", document->main()->height.get()));
        layer->transform->position.set(QPointF(
            -get(skeleton, "x", 0),
            get(skeleton, "y", 0)
        ));
        */
        layer->shapes.insert(load_skeleton(json));
        auto rect = layer->local_bounding_rect(0);
        document->main()->width.set(get(skeleton, "width", rect.width()));
        document->main()->height.set(get(skeleton, "height", rect.height()));
        layer->transform->position.set(-rect.topLeft());
        document->main()->shapes.insert(std::move(layer));
    }

    std::unique_ptr<model::Skeleton> load_skeleton(const QJsonObject& json)
    {
        skeleton = std::make_unique<model::Skeleton>(document);

        for ( const auto& b : json["bones"].toArray() )
            load_bone(b.toObject());

        for ( const auto& b : json["slots"].toArray() )
            load_slot(b.toObject());

        return std::move(skeleton);
    }

private:
    void load_bone(const QJsonObject& json)
    {
        auto bone = std::make_unique<model::Bone>(document);
        QString name = json["name"].toString();
        bones[name] = bone.get();
        bone->name.set(name);
        bone->display->length.set(get(json, "length", 0));
        bone->initial->position.set(QPointF(
            get(json, "x", 0),
            -get(json, "y", 0)
        ));
        bone->initial->scale.set(QVector2D(
            get(json, "scaleX", 1),
            get(json, "scaleY", 1)
        ));
        bone->initial->rotation.set(-get(json, "rotation", 0));
        bone->display->color.set(color(get(json, "color", "989898ff")));

        bone_parent(json["parent"].toString(), name)->insert(std::move(bone));
    }

    model::Bone* get_bone(const QString& name, const QString& error_name)
    {
        if ( name.isEmpty() )
            return nullptr;

        auto it = bones.find(name);
        if ( it == bones.end() )
        {
            warning(SpineFormat::tr("Reference to unexisting bone %1").arg(name), error_name);
            return nullptr;
        }

        return it->second;
    }

    model::ObjectListProperty<model::BoneItem>* bone_parent(const QString& name, const QString& error_name)
    {
        if ( auto bone = get_bone(name, error_name) )
            return &bone->children;
        return &skeleton->bones->values;
    }

    void load_slot(const QJsonObject& json)
    {
        auto slot = std::make_unique<model::SkinSlot>(document);
        QString name = json["name"].toString();
        skin_slots[name] = slot.get();
        slot->name.set(name);
        slot->group_color.set(color(get(json, "color", "ffffff00")));
        bone_parent(json["bone"].toString(), name)->insert(std::move(slot));
    }

    double get(const QJsonObject& json, const QString& key, double def)
    {
        auto it = json.find(key);
        if ( it == json.end() )
            return def;
        return it->toDouble();
    }

    QString get(const QJsonObject& json, const QString& key, const QString& def)
    {
        auto it = json.find(key);
        if ( it == json.end() )
            return def;
        return it->toString();
    }

    QColor color(const QString& spine_color)
    {
        QString qt_name = spine_color.left(6);
        if ( spine_color.size() > 6 )
            qt_name = spine_color.right(2) + qt_name;
        return QColor("#" + qt_name);
    }

    void warning(const QString& msg, const QString& where)
    {
        if ( ie )
            ie->warning(where + ": " + msg);
    }

    model::Document* document;
    std::unique_ptr<model::Skeleton> skeleton;
    std::map<QString, model::Bone*> bones;
    std::map<QString, model::SkinSlot*> skin_slots;
    io::ImportExport* ie;

};

} // namespace glaxnimate::io::spine
