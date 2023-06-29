/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "ae_project.hpp"
#include "model/document.hpp"
#include "aep_format.hpp"
#include "model/assets/assets.hpp"

namespace glaxnimate::io::aep {

class AepLoader
{
public:
    AepLoader(model::Document* document, const Project& project, QDir asset_path, ImportExport* io)
    : document(document), project(project), asset_path(asset_path), io(io)
    {}

    void load_project();

    struct CompData;
private:
    struct ColorInfo
    {
        model::NamedColor* asset;
        const Solid* solid;
    };

    void load_comp(const Composition& comp);
    void load_asset(const FolderItem* item);
    void load_layer(const Layer& layer, CompData& data);
    void asset_layer(model::Layer* layer, const Layer& ae_layer, CompData& data);
    void shape_layer(model::Layer* layer, const Layer& ae_layer, CompData& data);
    void text_layer(model::Layer* layer, const Layer& ae_layer, CompData& data);

    void warning(const QString& msg);
    void info(const QString& msg);
    model::Composition* get_comp(Id id);

    model::Document* document;
    const Project& project;
    QDir asset_path;
    ImportExport* io;
    std::unordered_map<Id, ColorInfo> colors;
    std::unordered_map<Id, model::Composition*> comps;
    std::unordered_map<Id, model::Bitmap*> images;
    std::unordered_map<Id, QPointF> asset_size;
};

} // namespace glaxnimate::io::aep
