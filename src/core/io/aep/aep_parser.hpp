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

public:
    Project parse(const RiffChunk& root)
    {
        if ( root.subheader != "Egg!" )
            throw AepError("Not an AEP file");

        Project project;
        Chunk fold = nullptr, efdg = nullptr;
        root.find_multiple({&fold, &efdg}, {"Fold", "EfdG"});

        if ( efdg )
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
                if ( current_item && child->data.read_uint<1>() )
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

                auto name = utf8_to_name(name_chunk);
                auto type = item->data.read_uint<2>();
                item->data.skip(14);
                auto id = item->data.read_uint<4>();

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
            warning(AepFormat::tr("Missing composition data"));

        auto data = cdta->data();
        // Time stuff
        data.skip(5);
        comp.time_scale = data.read_uint<2>();
        data.skip(14);
        comp.playhead_time = comp.time_to_frames(data.read_uint<2>());
        data.skip(6);
        comp.in_time = comp.time_to_frames(data.read_uint<2>());
        data.skip(6);
        auto out_time = data.read_uint<2>();
        data.skip(6);
        comp.duration = comp.time_to_frames(data.read_uint<2>());
        if ( out_time == 0xffff )
            comp.out_time = comp.duration;
        else
            comp.out_time = comp.time_to_frames(out_time);
        data.skip(5);

        // Background
        comp.color.setRed(data.read_uint<1>());
        comp.color.setGreen(data.read_uint<1>());
        comp.color.setBlue(data.read_uint<1>());

        // Lottie
        data.skip(85);
        comp.width = data.read_uint<2>();
        comp.height = data.read_uint<2>();
        data.skip(12);
        comp.framerate = data.read_uint<2>();

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

    QString utf8_to_name(Chunk utf8)
    {
        if ( !utf8 )
            return "";
        auto data = utf8->data().read();
        if ( data == placeholder )
            return "";
        return QString::fromUtf8(data);
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
            return nullptr;

        auto name = utf8_to_name(utf8);
        auto asset_reader = sspc->data();
        asset_reader.skip(32);
        auto width = asset_reader.read_uint<2>();
        asset_reader.skip(2);
        auto height = asset_reader.read_uint<2>();
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
        /// \todo
        return layer;
    }

    static constexpr const char* const placeholder = "-_0_/-";
    std::unordered_map<Id, Chunk> comp_chunks;
    // For loading into the object model stuff Glaxnimate doesn't support
    // I'm adding the code to load them just in case someone wants to do
    // something else with them and so that if Glaxnimate ever supports given
    // features, it's easier to add
    bool load_unecessary = false;
};

} // namespace glaxnimate::io::aep


