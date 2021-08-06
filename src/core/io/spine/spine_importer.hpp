#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QImageReader>

#include "model/shapes/layer.hpp"
#include "model/skeleton/skeleton.hpp"
#include "spine_format.hpp"

namespace glaxnimate::io::spine {


class SpineImporter
{
public:
    SpineImporter(model::Document* document, const QDir& search_path, io::ImportExport* ie)
        : document(document),
        ie(ie),
        search_path(search_path),
        search_path_images(search_path)
    {
    }

    void load_document(const QJsonObject& json)
    {
        QJsonObject skeleton = json["skeleton"].toObject();
        document->main()->fps.set(get(skeleton, "fps", 30));
        if ( skeleton.contains("images") )
            search_path_images.setPath(search_path.absoluteFilePath(skeleton["images"].toString()));

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

private:
    std::unique_ptr<model::Skeleton> load_skeleton(const QJsonObject& json)
    {
        skeleton = std::make_unique<model::Skeleton>(document);

        for ( const auto& b : json["bones"].toArray() )
            load_bone(b.toObject());

        int draw_order = 0;
        for ( const auto& b : json["slots"].toArray() )
            load_slot(b.toObject(), draw_order++);

        for ( const auto& b : json["skins"].toArray() )
            load_skin(b.toObject());

        if ( skeleton->skins->values.size() )
            skeleton->skin.set(skeleton->skins->values[0]);
        return std::move(skeleton);
    }

    void load_static_transform(model::StaticTransform* transform, const QJsonObject& json)
    {
        transform->position.set(QPointF(
            get(json, "x", 0),
            -get(json, "y", 0)
        ));
        transform->scale.set(QVector2D(
            get(json, "scaleX", 1),
            get(json, "scaleY", 1)
        ));
        transform->rotation.set(-get(json, "rotation", 0));
    }

    void load_bone(const QJsonObject& json)
    {
        auto bone = std::make_unique<model::Bone>(document);
        QString name = json["name"].toString();
        bones[name] = bone.get();
        bone->name.set(name);
        bone->display->length.set(get(json, "length", 0));
        load_static_transform(bone->initial.get(), json);
        if ( json.contains("color") )
            bone->display->color.set(color(json["color"].toString()));

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

    void load_slot(const QJsonObject& json, int draw_order)
    {
        auto slot = std::make_unique<model::SkinSlot>(document);
        QString name = json["name"].toString();
        skin_slots[name] = slot.get();
        slot->name.set(name);
        slot->group_color.set(color(get(json, "color", "ffffff00")));
        slot->draw_order.set(draw_order);
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

    void load_skin(const QJsonObject& json)
    {
        auto skin = std::make_unique<model::Skin>(document);
        QString name = json["name"].toString();
        skin->name.set(name);
        auto ptr = skin.get();
        skeleton->skins->values.insert(std::move(skin));

        auto att = json["attachments"].toObject();
        for ( auto it = att.begin(); it != att.end(); ++it  )
        {
            load_skin_item(ptr, it.key(), it->toObject());
        }
    }

    void load_skin_item(model::Skin* skin, const QString& slot_name, const QJsonObject& attachment_map)
    {
        static const std::map<QString, model::SkinItemBase* (SpineImporter::*)(model::Skin*, const QString&, const QJsonObject&)> methods = {
            {"region", &SpineImporter::load_region}
        };

        for ( auto mit = attachment_map.begin(); mit != attachment_map.end(); ++mit  )
        {
            auto json = mit->toObject();
            QString name = get(json, "name", mit.key());

            QString type = get(json, "type", "region");
            auto it = methods.find(type);
            if ( it == methods.end() )
            {
                warning(SpineFormat::tr("Unknown attachment type: %1").arg(type), name);
                continue;
            }

            auto item = (this->*it->second)(skin, name, json);
            if ( !item )
                continue;
            item->name.set(name);
            item->group_color.set(color(get(json, "color", "ffffff00")));
            auto slot_it = skin_slots.find(slot_name);
            if ( slot_it == skin_slots.end() )
                warning(SpineFormat::tr("Slot `%1` not found").arg(slot_name), name);
            else
                item->slot.set(slot_it->second);
        }
    }

    model::SkinItemBase* load_region(model::Skin* skin, const QString& name, const QJsonObject& json)
    {
        auto ptr = std::make_unique<model::ImageSkin>(document);
        auto item = ptr.get();
        skin->items.insert(std::move(ptr));

        item->image.set(load_image(name));
        load_static_transform(item->transform.get(), json);
        return item;
    }

    model::Bitmap* load_image(const QString& name)
    {
        auto it = images.find(name);
        if ( it != images.end() )
            return it->second;

        for ( const auto& entry : search_path_images.entryList({name + ".*"}, QDir::Files|QDir::Readable) )
        {
            QString filename = search_path_images.absoluteFilePath(entry);
            if ( !QImageReader::imageFormat(filename).isEmpty() )
            {
                auto image = std::make_unique<glaxnimate::model::Bitmap>(document);
                image->filename.set(filename);
                if ( image->pixmap().isNull() )
                {
                    warning(SpineFormat::tr("Could not load image"), name);
                    continue;
                }
                auto ptr = image.get();
                document->assets()->images->values.insert(std::move(image));
                images[name] = ptr;
                return ptr;
            }
        }

        warning(SpineFormat::tr("Image not found"), name);
        images[name] = nullptr;
        return nullptr;
    }

    model::Document* document;
    std::unique_ptr<model::Skeleton> skeleton;
    std::map<QString, model::Bone*> bones;
    std::map<QString, model::SkinSlot*> skin_slots;
    std::map<QString, model::Bitmap*> images;
    io::ImportExport* ie;
    QDir search_path;
    QDir search_path_images;
};

} // namespace glaxnimate::io::spine
