#pragma once

#include "ae_project.hpp"
#include "model/document.hpp"
#include "aep_format.hpp"

namespace glaxnimate::io::aep {

class AepLoader
{
public:
    AepLoader(model::Document* doc, const Project& project, QDir asset_path, ImportExport* io)
    : doc(doc), project(project), asset_path(asset_path), io(io)
    {}

    void load_project(){}

private:
    model::Document* doc;
    const Project& project;
    QDir asset_path;
    ImportExport* io;
};

} // namespace glaxnimate::io::aep
