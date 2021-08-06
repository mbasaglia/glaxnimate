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

        if ( max_ft > 0 )
        {
            document->main()->animation->last_frame.set(max_ft);
            layer->animation->last_frame.set(max_ft);
        }

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

        if ( json.contains("events") && !json["events"].toArray().empty() )
            warning(SpineFormat::tr("Events are not supported"), SpineFormat::tr("Skeleton"));

        auto anim = json["animations"].toObject();
        if ( anim.length() > 0 )
        {
            auto anim_name = anim.keys()[0];
            if ( anim.length() > 1 )
                warning(SpineFormat::tr("Only one animation will be loaded"), anim_name);
            load_animation(anim_name, anim[anim_name].toObject());
        }

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

    static double get(const QJsonObject& json, const QString& key, double def)
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

    model::SkinSlot* slot(const QString& slot_name, const QString& error_name)
    {
        auto slot_it = skin_slots.find(slot_name);
        if ( slot_it != skin_slots.end() )
            return slot_it->second;
        warning(SpineFormat::tr("Slot `%1` not found").arg(slot_name), error_name);
        return nullptr;
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
            item->slot.set(slot(slot_name, name));
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

    void load_animation(const QString& anim_name, const QJsonObject& json)
    {
        load_animation_slots(anim_name, json["slots"].toObject());
        load_animation_bones(anim_name, json["bones"].toObject());
    }

    void load_animation_slots(const QString& anim_name, const QJsonObject& json)
    {
        for ( auto it =  json.begin(); it != json.end(); ++it )
        {
            const QString& slot_name = it.key();
            QJsonObject janim = it->toObject();
            if ( auto slot = this->slot(slot_name, anim_name) )
                load_animation_slot(slot, janim);
        }
    }

    void load_animation_slot(model::SkinSlot* slot, const QJsonObject& json)
    {
        for ( auto it = json.begin(); it != json.end(); ++it )
        {
            // docs says color but json says rgba
            if ( it.key() != "color" && it.key() != "rgba" )
            {
                warning(SpineFormat::tr("Slot %1 animation not supported").arg(it.key()), slot->object_name());
                continue;
            }
            load_keyframes(it->toArray(), slot->opacity, [](const QJsonObject& j){
                return float(j["color"].toString().right(2).toInt(nullptr, 16)) / 0xff;
            });
        }
    }

    void load_animation_bones(const QString& anim_name, const QJsonObject& json)
    {
        for ( auto it =  json.begin(); it != json.end(); ++it )
        {
            const QString& bone_name = it.key();
            QJsonObject janim = it->toObject();
            if ( auto bone = get_bone(bone_name, anim_name) )
                load_animation_bone(bone, janim);
        }
    }

    void load_animation_bone(model::Bone* bone, const QJsonObject& json)
    {
        for ( auto it = json.begin(); it != json.end(); ++it )
        {
            auto frames = it->toArray();
            if ( it.key() == "translate" )
                load_keyframes(frames, bone->pose->position, &SpineImporter::load_value_point);
            else if ( it.key() == "rotate" )
                load_keyframes(frames, bone->pose->rotation, &SpineImporter::load_value_angle);
            else if ( it.key() == "scale" )
                load_keyframes(frames, bone->pose->scale, &SpineImporter::load_value_scale);
            else
                warning(SpineFormat::tr("Bone %1 animation not supported").arg(it.key()), bone->object_name());
        }
    }

    static QPointF load_value_point(const QJsonObject& js)
    {
        return {get(js, "x", 0), -get(js, "y", 0)};
    }

    static QVector2D load_value_scale(const QJsonObject& js)
    {
        return QVector2D(get(js, "x", 1), get(js, "y", 1));
    }

    static float load_value_angle(const QJsonObject& js)
    {
        return -get(js, "value", 0);
    }

    template<class T, class Callback>
    void load_keyframes(const QJsonArray& json, model::AnimatedProperty<T>& prop, const Callback& callback)
    {
        for ( const auto& kfv : json )
        {
            auto kfj = kfv.toObject();
            model::FrameTime t = kfj["time"].toDouble() * document->main()->fps.get();
            if ( t > max_ft )
                max_ft = t;

            auto kf = prop.set_keyframe(t, callback(kfj));
            model::KeyframeTransition transition;
            if ( kfj["curve"].toString() == "stepped" )
            {
                transition.set_after_descriptive(model::KeyframeTransition::Hold);
            }
            else
            {
                transition.set_before(QPointF(
                    get(kfj, "curve", 0),
                    get(kfj, "c2", 0)
                ));

                transition.set_before(QPointF(
                    get(kfj, "c3", 1),
                    get(kfj, "c4", 1)
                ));
            }
            kf->set_transition(transition);
        }
    }

    model::Document* document;
    std::unique_ptr<model::Skeleton> skeleton;
    std::map<QString, model::Bone*> bones;
    std::map<QString, model::SkinSlot*> skin_slots;
    std::map<QString, model::Bitmap*> images;
    io::ImportExport* ie;
    QDir search_path;
    QDir search_path_images;
    model::FrameTime max_ft = 0;
};

} // namespace glaxnimate::io::spine
