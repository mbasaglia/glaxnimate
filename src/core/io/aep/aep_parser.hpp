/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QJsonDocument>
#include <QJsonObject>

#include "aep_riff.hpp"
#include "ae_project.hpp"
#include "cos.hpp"
#include "gradient_xml.hpp"
#include "aep_format.hpp"

namespace glaxnimate::io::aep {

class AepError : public std::runtime_error
{
public:
    AepError(QString message) : runtime_error(message.toStdString()), message(std::move(message)) {}

    QString message;
};

class AepParser
{
private:
    using Chunk = const RiffChunk*;
    using ChunkRange = RiffChunk::FindRange;
    struct PropertyContext
    {
        Composition* comp = nullptr;
        Layer* layer = nullptr;

        model::FrameTime time_to_frames(model::FrameTime time) const
        {
            return (time + layer->start_time) / comp->time_scale;
        }
    };

public:
    Project parse(const RiffChunk& root)
    {
        if ( root.subheader != "Egg!" )
            throw AepError("Not an AEP file");

        Project project;
        Chunk fold = nullptr, efdg = nullptr;
        root.find_multiple({&fold, &efdg}, {"Fold", "EfdG"});

        if ( load_unecessary && efdg )
            parse_effects(efdg->find_all("EfDf"), project);

        parse_folder(fold, project.folder, project);

        for ( auto& comp : project.compositions )
            parse_composition(comp_chunks[comp->id], *comp, project);

        return project;
    }

private:
    void parse_folder(Chunk chunk, Folder& folder, Project& project)
    {
        FolderItem* current_item = nullptr;

        for ( const auto& child : chunk->children )
        {
            if ( *child == "fiac" )
            {
                if ( current_item && child->data().read_uint8() )
                    project.current_item = current_item;
            }
            else if ( *child == "Item" )
            {
                Chunk item = nullptr;
                Chunk name_chunk = nullptr;
                child->find_multiple({&item, &name_chunk}, {"idta", "Utf8"});
                current_item = nullptr;

                if ( !item )
                    continue;

                auto name = to_string(name_chunk);
                auto data = item->data();
                auto type = data.read_uint16();
                data.skip(14);
                auto id = data.read_uint32();

                switch ( type )
                {
                    case 1: // Folder
                    {
                        auto child_item = folder.add<Folder>();
                        child_item->id = id;
                        child_item->name = name;
                        current_item = child_item;
                        parse_folder(child.get(), *child_item, project);
                        break;
                    }
                    case 4: // Composition
                    {
                        auto comp = folder.add<Composition>();
                        comp->id = id;
                        comp->name = name;
                        current_item = comp;
                        project.compositions.push_back(comp);
                        project.assets[id] = comp;
                        comp_chunks[id] = child.get();
                        break;
                    }
                    case 7: // Asset
                        current_item = parse_asset(id, child->child("Pin "), folder, project);
                        break;
                    default:
                        warning(QObject::tr("Unknown Item type %s").arg(type));
                }
            }
        }
    }

    void parse_effects(const ChunkRange& range, Project& project)
    {
        /// \todo
    }

    void parse_composition(Chunk chunk, Composition& comp, Project& project)
    {
        auto cdta = chunk->child("cdta");
        if ( !cdta )
        {
            warning(AepFormat::tr("Missing composition data"));
            return;
        }

        auto data = cdta->data();
        // Time stuff
        data.skip(5);
        comp.time_scale = data.read_uint16();
        data.skip(14);
        comp.playhead_time = comp.time_to_frames(data.read_uint16());
        data.skip(6);
        comp.in_time = comp.time_to_frames(data.read_uint16());
        data.skip(6);
        auto out_time = data.read_uint16();
        data.skip(6);
        comp.duration = comp.time_to_frames(data.read_uint16());
        if ( out_time == 0xffff )
            comp.out_time = comp.duration;
        else
            comp.out_time = comp.time_to_frames(out_time);
        data.skip(5);

        // Background
        comp.color.setRed(data.read_uint8());
        comp.color.setGreen(data.read_uint8());
        comp.color.setBlue(data.read_uint8());

        // Lottie
        data.skip(85);
        comp.width = data.read_uint16();
        comp.height = data.read_uint16();
        data.skip(12);
        comp.framerate = data.read_uint16();

        for ( const auto& child : chunk->children )
        {
            if ( *child == "Layr" )
                comp.layers.push_back(parse_layer(child.get(), comp));
            else if ( load_unecessary && *child == "SecL" )
                comp.markers = parse_layer(child.get(), comp);
            else if ( load_unecessary && (*child == "CLay" || *child == "DLay" || *child == "SLay") )
                comp.views.push_back(parse_layer(child.get(), comp));
        }
    }

    QString to_string(Chunk chunk)
    {
        if ( !chunk )
            return "";
        auto data = chunk->data().read();
        if ( data == placeholder )
            return "";

        if ( chunk->header == "Utf8" )
            return QString::fromUtf8(data);

        warning(AepFormat::tr("Unknown encoding for %1").arg(chunk->header.to_string()));
        return "";
    }

    void warning(const QString& msg) const
    {
        /// \todo
    }

    FolderItem* parse_asset(Id id, Chunk chunk, Folder& folder, Project& project)
    {
        Chunk sspc, utf8, als2, opti;
        sspc = utf8 = als2 = opti = nullptr;
        chunk->find_multiple({&sspc, &utf8, &als2, &opti}, {"sspc", "Utf8", "Als2", "opti"});
        if ( !sspc || !opti )
        {
            warning(AepFormat::tr("Missing asset data"));
            return nullptr;
        }

        auto name = to_string(utf8);
        auto asset_reader = sspc->data();
        asset_reader.skip(32);
        auto width = asset_reader.read_uint16();
        asset_reader.skip(2);
        auto height = asset_reader.read_uint16();
        Asset* asset;

        auto data = opti->data();

        if ( data.read(4) == "Soli" )
        {
            auto solid = folder.add<Solid>();
            solid->color.setAlphaF(data.read_float32());
            solid->color.setRedF(data.read_float32());
            solid->color.setGreenF(data.read_float32());
            solid->color.setBlueF(data.read_float32());
            solid->name = data.read_utf8_nul(256);
            asset = solid;
        }
        else
        {
            auto doc = QJsonDocument::fromJson(als2->child("alas")->data().read());
            QString path = doc.object()["fullpath"].toString();
            // Handle weird windows paths
            if ( path.contains('\\') && QDir::separator() == '/' )
            {
                path = path.replace('\\', '/');
                if ( path.size() > 1 && path[1] == ':' )
                    path = '/' + path;
            }
            auto file = folder.add<FileAsset>();
            file->path = QFileInfo(path);
            file->name = name.isEmpty() ? file->path.fileName() : name;
            asset = file;
        }

        asset->width = width;
        asset->height = height;
        project.assets[id] = asset;
        return asset;
    }

    std::unique_ptr<Layer> parse_layer(Chunk chunk, Composition& comp)
    {
        auto layer = std::make_unique<Layer>();

        Chunk ldta, utf8, tdgp;
        ldta = utf8 = tdgp = nullptr;
        chunk->find_multiple({&ldta, &utf8, &tdgp}, {"ldta", "Utf8", "tdgp"});
        if ( !ldta )
        {
            warning(AepFormat::tr("Missing layer data"));
            return {};
        }

        auto data = ldta->data();
        PropertyContext context{&comp, layer.get()};

        layer->name = to_string(utf8);
        layer->id = data.read_uint32();
        layer->quality = LayerQuality(data.read_uint16());
        data.skip(7);
        layer->start_time = comp.time_to_frames(data.read_sint<2>());
        data.skip(6);
        layer->in_time = context.time_to_frames(data.read_uint16());
        data.skip(6);
        layer->out_time = context.time_to_frames(data.read_uint16());
        data.skip(6);
        Flags flags = data.read_uint<3>();
        layer->is_guide = flags.get(2, 1);
        layer->bicubic_sampling = flags.get(2, 6);
        layer->auto_orient = flags.get(1, 0);
        layer->is_adjustment = flags.get(1, 1);
        layer->threedimensional = flags.get(1, 2);
        layer->solo = flags.get(1, 3);
        layer->is_null = flags.get(1, 7);
        layer->visible = flags.get(0, 0);
        layer->effects_enabled = flags.get(0, 2);
        layer->motion_blur = flags.get(0, 3);
        layer->locked = flags.get(0, 5);
        layer->shy = flags.get(0, 6);
        layer->continuously_rasterize = flags.get(0, 7);
        layer->asset_id = data.read_uint32();
        data.skip(17);
        layer->label_color = LabelColors(data.read_uint8());
        data.skip(2);
        data.skip(32); // Name, we get it from Utf8 instead
        data.skip(11);
        layer->matte_mode = TrackMatteType(data.read_uint8());
        data.skip(23);
        layer->type = LayerType(data.read_uint8());
        layer->parent_id = data.read_uint32();
        data.skip(24);
        layer->matte_id = data.read_uint32();

        parse_property_group(tdgp, layer->properties, context);

        return layer;
    }

    void parse_property_group(Chunk chunk, PropertyGroup& group, const PropertyContext& context)
    {
        QString match_name;
        for ( auto it = chunk->children.begin(); it != chunk->children.end(); ++it )
        {
            auto child = it->get();

            if ( *child == "tdmn" )
            {
                match_name = child->data().read_utf8_nul();
            }
            else if ( *child == "tdsb" )
            {
                Flags flags = child->data().read_uint32();
                group.visible = flags.get(0, 0);
                group.split_position = flags.get(0, 1);
            }
            else if ( *child == "tdsn" )
            {
                group.name = to_string(child->child("Utf8"));
            }
            else if ( *child == "mkif" )
            {
                auto mask = std::make_unique<Mask>();
                auto data = child->data();
                mask->inverted = data.read_uint8();
                mask->locked = data.read_uint8();
                data.skip(4);
                mask->mode = MaskMode(data.read_uint16());
                ++it;
                if ( it == chunk->children.end() )
                {
                    warning(AepFormat::tr("Missing mask properties"));
                    return;
                }
                if ( **it != "tdgp" )
                {
                    warning(AepFormat::tr("Missing mask properties"));
                    continue;
                }

                parse_property_group(it->get(), mask->properties, context);
                group.properties.push_back({match_name, std::move(mask)});
                match_name.clear();
            }
            else if ( !match_name.isEmpty() )
            {
                auto prop = parse_property(child, context);
                if ( prop )
                    group.properties.push_back({match_name, std::move(prop)});
                match_name.clear();
            }
        }
    }

    std::unique_ptr<PropertyGroup> parse_property_group(Chunk chunk, const PropertyContext& context)
    {
        auto group = std::make_unique<PropertyGroup>();
        parse_property_group(chunk, *group, context);
        return group;
    }

    std::unique_ptr<PropertyBase> parse_property(Chunk chunk, const PropertyContext& context)
    {
        if ( *chunk == "tdgp" )
            return parse_property_group(chunk, context);
        else if ( *chunk == "tdbs" )
            return parse_animated_property(chunk, context, {});
        else if ( *chunk == "om-s" )
            return parse_animated_with_values(chunk, context, "omks", "shap", &AepParser::parse_bezier);
        else if ( *chunk == "GCst" )
            return parse_animated_with_values(chunk, context, "GCky", "Utf8", &AepParser::parse_gradient);
        else if ( *chunk == "btds" )
            return parse_animated_text(chunk, context);
        else if ( *chunk == "sspc" )
            return parse_effect_instance(chunk, context);
        else if ( *chunk == "otst" )
            return load_unecessary ? parse_animated_with_values(chunk, context, "otky", "otda", &AepParser::parse_orientation) : nullptr;
        else if ( *chunk == "mrst" )
            return load_unecessary ? parse_animated_with_values(chunk, context, "mrky", "Nmrd", &AepParser::parse_marker) : nullptr;
        // I've seen these in files but I'm not sure how to parse them
        else if ( *chunk == "OvG2" || *chunk == "blsi" || *chunk == "blsv" )
            return {};

        warning(AepFormat::tr("Unknown property type: %1").arg(chunk->name().to_string()));
    }

    std::unique_ptr<Property> parse_animated_property(
        Chunk chunk, const PropertyContext& context, std::vector<PropertyValue> values
    )
    {
        auto prop = std::make_unique<Property>();
        Chunk header, value, keyframes, expression, tdpi, tdps, tdli;
        header = value = keyframes = expression = tdpi = tdps = tdli = nullptr;
        chunk->find_multiple(
            {&header, &value, &keyframes, &expression, &tdpi, &tdps, &tdli},
            {"tdb4", "cdat", "list", "Utf8", "tdpi", "tdps", "tdli"}
        );
        auto data = header->data();
        data.skip(2);
        prop->components = data.read_uint16();

        bool position = Flags(data.read_uint16()).get(0, 3);
        data.skip(10+8*5);
        Flags type = data.read_uint32();
        bool no_value = type.get(2, 0);
        bool color = type.get(0, 0);
        bool integer = type.get(0, 2);
        data.skip(8);

        if ( position )
            prop->type = PropertyType::Position;
        else if ( color )
            prop->type = PropertyType::Color;
        else if ( no_value )
            prop->type = PropertyType::NoValue;
        else if ( integer )
            prop->type = PropertyType::Integer;
        else
            prop->type = PropertyType::MultiDimensional;

        prop->animated = data.read_uint8() == 1;

        if ( integer && tdpi )
        {
            prop->type = PropertyType::LayerSelection;
            LayerSelection val;
            val.layer_id = tdpi->data().read_uint32();
            if ( tdps )
                val.layer_source = LayerSource(tdps->data().read_sint<4>());
            prop->value = val;
        }
        else if ( integer && tdli )
        {
            prop->type = PropertyType::MaskIndex;
            prop->value = tdli->data().read_uint32();
        }
        else if ( keyframes )
        {
            auto raw_keys = list_values(keyframes);
            for ( std::size_t i = 0; i < raw_keys.size(); i++ ) {
                prop->keyframes.push_back(load_keyframe(i, raw_keys[i], *prop, context, values));
            }
        }
        else if ( value )
        {
            auto vdat = value->data();
            auto raw_value = vdat.read_array(&BinaryReader::read_float64, prop->components);
            prop->value = property_value(0, raw_value, values, prop->type);
        }

        if ( expression )
            prop->expression = to_string(expression);

        return prop;
    }

    PropertyValue property_value(
        int index,
        const std::vector<qreal>& raw_value,
        std::vector<PropertyValue>& values,
        PropertyType type
    )
    {
        switch ( type )
        {
            case PropertyType::NoValue:
                if ( index < int(values.size()) )
                    return std::move(values[index]);
                return nullptr;
            case PropertyType::Color:
                if ( raw_value.size() < 4 )
                    return QColor();
                return QColor(raw_value[1], raw_value[2], raw_value[3], raw_value[0]);
            default:
                return vector_value(raw_value);
        }
    }

    PropertyValue vector_value(const std::vector<qreal>& raw_value)
    {
        switch ( raw_value.size() )
        {
            case 0:
                return nullptr;
            case 1:
                return raw_value[0];
            case 2:
                return QPointF(raw_value[0], raw_value[1]);
            case 3:
            default:
                return QVector3D(raw_value[0], raw_value[1], raw_value[2]);
        }
    }

    Keyframe load_keyframe(int index, BinaryReader& reader, Property& prop, const PropertyContext& context, std::vector<PropertyValue>& values)
    {
        Keyframe kf;

        reader.skip(1);
        kf.time = context.time_to_frames(reader.read_uint16());
        reader.skip(2);

        kf.transition_type = KeyframeTransitionType(reader.read_uint8());

        kf.label_color = LabelColors(reader.read_uint8());

        Flags flags  = reader.read_uint8();
        kf.roving = flags.get(0, 5);
        if ( flags.get(0, 3) )
            kf.bezier_mode = KeyframeBezierMode::Continuous;
        else if ( flags.get(0, 4) )
            kf.bezier_mode = KeyframeBezierMode::Auto;
        else
            kf.bezier_mode = KeyframeBezierMode::Normal;

        if ( prop.type == PropertyType::NoValue )
        {
            reader.skip(16);
            kf.in_speed.push_back(reader.read_float64());
            kf.in_influence.push_back(reader.read_float64());
            kf.out_speed.push_back(reader.read_float64());
            kf.out_influence.push_back(reader.read_float64());
            kf.value = std::move(values[index]);
        }
        else if ( prop.type == PropertyType::MultiDimensional || prop.type == PropertyType::Integer )
        {
            kf.value = vector_value(reader.read_array(&BinaryReader::read_float64, prop.components));
            kf.in_speed = reader.read_array(&BinaryReader::read_float64, prop.components);
            kf.in_influence = reader.read_array(&BinaryReader::read_float64, prop.components);
            kf.out_speed = reader.read_array(&BinaryReader::read_float64, prop.components);
            kf.out_influence = reader.read_array(&BinaryReader::read_float64, prop.components);
        }
        else if ( prop.type == PropertyType::Position )
        {
            reader.skip(16);
            kf.in_speed.push_back(reader.read_float64());
            kf.in_influence.push_back(reader.read_float64());
            kf.out_speed.push_back(reader.read_float64());
            kf.out_influence.push_back(reader.read_float64());
            kf.value = vector_value(reader.read_array(&BinaryReader::read_float64, prop.components));
            auto it = reader.read_array(&BinaryReader::read_float64, prop.components);
            auto ot = reader.read_array(&BinaryReader::read_float64, prop.components);
            if ( prop.components >= 2 )
            {
                kf.in_tangent = {it[0], it[1]};
                kf.out_tangent = {ot[0], ot[1]};
            }
        }
        else if ( prop.type == PropertyType::Color )
        {
            reader.skip(16);
            kf.in_speed.push_back(reader.read_float64());
            kf.in_influence.push_back(reader.read_float64());
            kf.out_speed.push_back(reader.read_float64());
            kf.out_influence.push_back(reader.read_float64());
            auto value = reader.read_array(&BinaryReader::read_float64, prop.components);
            kf.value = QColor(value[1], value[2], value[3], value[0]);
        }

        return kf;
    }

    template<class T>
    std::unique_ptr<Property> parse_animated_with_values(
        Chunk chunk, const PropertyContext& context,
        const char* container, const char* value_name,
        T (AepParser::*parse)(Chunk chunk)
    )
    {
        Chunk value_container, tdbs;
        value_container = tdbs = nullptr;
        chunk->find_multiple({&value_container, &tdbs}, {container, "tdbs"});
        std::vector<PropertyValue> values;
        for ( const RiffChunk& value_chunk : value_container->find_all(value_name) )
            values.emplace_back((this->*parse)(&value_chunk));
        return parse_animated_property(tdbs, context, std::move(values));
    }

    std::vector<BinaryReader> list_values(Chunk list)
    {
        Chunk head, vals;
        head = vals = nullptr;
        list->find_multiple({&head, &vals}, {"lhd3", "ldat"});
        if ( !head || !vals )
        {
            warning(AepFormat::tr("Missing list data"));
            return {};
        }

        auto data = head->data();
        data.skip(10);
        std::uint32_t count = data.read_uint16();
        data.skip(6);
        std::uint32_t size = data.read_uint16();
        std::uint32_t total_size = count * size;
        if ( vals->reader.size() < total_size )
        {
            warning(AepFormat::tr("Not enough data in list"));
            return {};
        }

        std::vector<BinaryReader> values;
        values.reserve(count);
        for ( std::uint32_t i = 0; i < count; i++ )
            values.push_back(vals->reader.sub_reader(size, i * size));
        return values;
    }

    BezierData parse_bezier(Chunk chunk)
    {
        BezierData data;
        auto bounds = chunk->child("shph")->data();
        bounds.skip(3);
        data.closed = Flags(bounds.read_uint8()).get(0, 3);
        data.minimum.setX(bounds.read_float32());
        data.minimum.setY(bounds.read_float32());
        data.maximum.setX(bounds.read_float32());
        data.maximum.setY(bounds.read_float32());

        for ( auto& pt : list_values(chunk->child("list")) )
        {
            float x = pt.read_float32();
            float y = pt.read_float32();
            data.points.push_back({x, y});
        }

        return data;
    }

    Gradient parse_gradient(Chunk chunk)
    {
        return parse_gradient_xml(to_string(chunk));
    }

    QVector3D parse_orientation(Chunk chunk)
    {
        auto data = chunk->data();
        QVector3D v;
        v.setX(data.read_float64());
        v.setY(data.read_float64());
        v.setZ(data.read_float64());
        return v;
    }

    Marker parse_marker(Chunk chunk)
    {
        Marker marker;
        marker.name = to_string(chunk->child("Utf8"));
        auto data = chunk->child("NmHd")->data();
        data.skip(4);
        marker.is_protected = data.read_uint8() & 2;
        data.skip(4);
        marker.duration = data.read_uint32();
        data.skip(4);
        marker.label_color = LabelColors(data.read_uint8());
        return marker;
    }

    std::unique_ptr<Property> parse_animated_text(Chunk chunk, const PropertyContext& context)
    {
        /// \todo
    }

    std::unique_ptr<EffectInstance> parse_effect_instance(Chunk chunk, const PropertyContext& context)
    {
        if ( !load_unecessary )
            return {};
        /// \todo
    }

    static constexpr const char* const placeholder = "-_0_/-";
    std::unordered_map<Id, Chunk> comp_chunks;
    // For loading into the object model stuff Glaxnimate doesn't support
    // I'm adding the code to load them just in case someone wants to do
    // something else with them and so that if Glaxnimate ever supports given
    // features, it's easier to add
    const bool load_unecessary = false;
};

} // namespace glaxnimate::io::aep


