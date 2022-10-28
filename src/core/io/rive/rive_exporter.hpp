#pragma once

#include "rive_serializer.hpp"

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "rive_format.hpp"

namespace glaxnimate::io::rive {

class RiveExporter
{

public:
    explicit RiveExporter(QIODevice* file, ImportExport* format)
        : serializer(file), format(format)
    {
        serializer.write_header(7, 0, 0);
        serializer.write_property_table({});
        write_object(TypeId::Backboard);
    }

    void write_document(model::Document* document)
    {
        write_assets(document->assets()->images.get());

        write_composition(document->main(), document->size());

        for ( const auto& comp : document->assets()->precompositions->values )
            write_composition(comp.get(), document->size());
    }

private:
    void write_assets(const model::BitmapList* assets)
    {
        for ( const auto& image : assets->values )
        {
            write_bitmap(image.get());
        }
    }

    void write_bitmap(model::Bitmap* image)
    {
        auto name = image->name.get();
        if ( name.isEmpty() )
            name = image->filename.get();

        // idk what this is used for, let's just set it to a unique value the lazy way
        Identifier asset_id = reinterpret_cast<Identifier>(image);

        auto obj = types.object(TypeId::ImageAsset);
        if ( !obj )
            return;
        object_ids[image] = next_asset++;

        obj.set("name", name);
        obj.set("width", image->width.get());
        obj.set("height", image->height.get());
        obj.set("assetId", asset_id);

        serializer.write_object(obj);

        auto data = image->image_data();
        if ( !data.isEmpty() )
        {
            auto contents = types.object(TypeId::FileAssetContents);
            if ( !contents )
                return;
            obj.set("bytes", data);
        }
    }

    bool write_object(TypeId type, const QVariantMap& props = {})
    {
        auto obj = types.object(type);
        if ( !obj )
            return false;

        for ( auto it = props.begin(); it != props.end(); ++it )
            obj.set(it.key(), *it);

        serializer.write_object(obj);

        return true;
    }

    void write_composition(model::Composition* comp, QSizeF size)
    {
        object_ids[comp] = next_artboard++;
        next_artboard_child = 1;
        animations.clear();

        if ( !write_object(TypeId::Artboard, {
            {"name", comp->name.get()},
            {"width", size.width()},
            {"height", size.height()},
            {"x", (24 + size.width()) * (next_artboard - 1)}
        }) ) return;

        for ( const auto& shape : comp->shapes )
            write_shape(shape.get(), 0);

        write_object(TypeId::LinearAnimation, {{"loopValue", 1}});
        for ( const auto& obj : animations )
            serializer.write_object(obj);
        write_object(TypeId::StateMachine, {});
        write_object(TypeId::StateMachineLayer, {});
        write_object(TypeId::AnimationState, {{"animationId", 0}});
        write_object(TypeId::EntryState, {});
        write_object(TypeId::StateTransition, {{"stateToId", 0}});
        write_object(TypeId::AnyState, {});
        write_object(TypeId::ExitState, {});
    }

    void write_shape(model::ShapeElement* shape, Identifier parent_id)
    {
        auto id = next_artboard_child++;
        object_ids[shape] = id;

        /*if ( auto layer = shape->cast<model::Layer>() )
        {
        }
        else*/
        {
            write_object(TypeId::Shape, {
                {"name", shape->name.get()},
                {"parentId", QVariant::fromValue(parent_id)},
            });
        }
    }

    Identifier next_asset = 0;
    Identifier next_artboard = 0;
    Identifier next_artboard_child = 0;
    std::unordered_map<model::DocumentNode*, Identifier> object_ids;
    RiveSerializer serializer;
    ImportExport* format;
    std::vector<Object> animations;
    TypeSystem types;
};

} // namespace glaxnimate::io::rive
