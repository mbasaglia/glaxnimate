#include "aep_loader.hpp"


using namespace glaxnimate::io::aep;
using namespace glaxnimate;

void glaxnimate::io::aep::AepLoader::load_project()
{
    for ( const auto& pair : project.assets )
        load_asset(pair.second);

    for ( const auto& comp : project.compositions )
        load_comp(*comp);
}

void glaxnimate::io::aep::AepLoader::load_asset(const glaxnimate::io::aep::FolderItem* item)
{
    if ( item->type() == FolderItem::Asset )
    {
        auto image = std::make_unique<glaxnimate::model::Bitmap>(document);
        auto asset = static_cast<const FileAsset*>(item);
        if ( asset->path.exists() )
        {
            image->filename.set(asset->path.filePath());
        }
        else
        {
            // Handle collected assets
            QFileInfo path = asset_path.filePath(asset->path.fileName());
            if ( !path.exists() )
                warning(AepFormat::tr("External asset not found: %1").arg(asset->path.filePath()));
            else
                image->filename.set(path.filePath());
        }
        images[item->id] = image.get();
        document->assets()->images->values.insert(std::move(image));
    }
    else if ( item->type() == FolderItem::Solid )
    {
        auto color = std::make_unique<glaxnimate::model::NamedColor>(document);
        auto solid = static_cast<const Solid*>(item);
        color->color.set(solid->color);
        colors[item->id] = {color.get(), solid};
        document->assets()->colors->values.insert(std::move(color));
    }
}

void glaxnimate::io::aep::AepLoader::load_comp(const glaxnimate::io::aep::Composition& comp)
{
}

void glaxnimate::io::aep::AepLoader::warning(const QString& msg)
{
    io->warning(msg);
}



