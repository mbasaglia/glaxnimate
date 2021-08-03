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
        document->main()->width.set(get(skeleton, "width", document->main()->width.get()));
        document->main()->height.set(get(skeleton, "height", document->main()->height.get()));
        document->main()->fps.set(get(skeleton, "fps", 30));
        auto layer = std::make_unique<model::Layer>(document);
        layer->transform->position.set(QPointF(
            -get(skeleton, "x", 0),
            get(skeleton, "y", 0)
        ));
        layer->shapes.insert(load_skeleton(json));
        document->main()->shapes.insert(std::move(layer));
    }

    std::unique_ptr<model::Skeleton> load_skeleton(const QJsonObject& json)
    {
        skeleton = std::make_unique<model::Skeleton>(document);
        load_bones(json["bones"].toArray());
        return std::move(skeleton);
    }

private:
    void load_bones(const QJsonArray& arr)
    {
        for ( const auto& b : arr )
            load_bone(b.toObject());
    }

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

        QString parent_name = json["parent"].toString();
        model::Bone* parent = nullptr;
        if ( !parent_name.isEmpty() )
        {
            auto it = bones.find(parent_name);
            if ( it == bones.end() )
                warning(SpineFormat::tr("Missing parent %1").arg(parent_name), name);
            else
                parent = it->second;
        }
        if ( parent )
            parent->children.insert(std::move(bone));
        else
            skeleton->bones->values.insert(std::move(bone));
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
    io::ImportExport* ie;

};

} // namespace glaxnimate::io::spine
