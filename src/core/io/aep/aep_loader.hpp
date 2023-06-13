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

private:
    void load_comp(const Composition& comp);
    void load_asset(const FolderItem* item);
    void warning(const QString& msg);

    struct ColorInfo
    {
        model::NamedColor* asset;
        const Solid* solid;
    };

    model::Document* document;
    const Project& project;
    QDir asset_path;
    ImportExport* io;
    std::unordered_map<Id, ColorInfo> colors;
    std::unordered_map<Id, model::Composition*> comps;
    std::unordered_map<Id, model::Bitmap*> images;
};

} // namespace glaxnimate::io::aep
