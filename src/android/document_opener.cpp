/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "document_opener.hpp"

#include <QMessageBox>

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "utils/gzip.hpp"
#include "model/assets/assets.hpp"
#include "model/shapes/image.hpp"

#include "widgets/dialogs/trace_dialog.hpp"

#include "android_file_picker.hpp"
#include "base_dialog.hpp"

#include <QDebug>
class glaxnimate::android::DocumentOpener::Private
{
public:
    QWidget* widget_parent;

    bool do_save_document(model::Document *document, const io::Options& opts, QIODevice& file)
    {
        return opts.format->save(file, opts.filename, document, opts.settings);
    }

    std::unique_ptr<model::Document> from_raster(const QByteArray& data)
    {
        auto btn = QMessageBox::information(
            widget_parent,
            QObject::tr("Open File"),
            QObject::tr("Raster images need to be traced into vectors"),
            QMessageBox::Ok|QMessageBox::Cancel
        );
        if ( btn == QMessageBox::Cancel )
            return {};

        auto doc = std::make_unique<model::Document>("");
        auto asset = doc->assets()->add_image(QImage::fromData(data));
        auto imageu = std::make_unique<model::Image>(doc.get());
        auto image = imageu.get();
        doc->main()->shapes.insert(std::move(imageu));
        doc->main()->width.set(asset->width.get());
        doc->main()->height.set(asset->height.get());
        QPointF pos(asset->width.get()/2., asset->height.get()/2.);
        image->transform->anchor_point.set(pos);
        image->transform->position.set(pos);
        image->image.set(asset);

        gui::TraceDialog dialog(image, widget_parent);
        DialogFixerFilter fixer(&dialog);
        if ( !dialog.exec() )
            return {};

        doc->assets()->images->values.remove(0);
        doc->main()->shapes.remove(doc->main()->shapes.index_of(image));

        return doc;
    }

};

glaxnimate::android::DocumentOpener::DocumentOpener(QWidget *widget_parent)
    : d(std::make_unique<Private>())
{
    d->widget_parent = widget_parent;
}

glaxnimate::android::DocumentOpener::~DocumentOpener()
{

}

bool glaxnimate::android::DocumentOpener::save(const QUrl &url, model::Document *document, io::Options &options) const
{
    if ( !url.isValid() )
        return false;

    QString path = url.path();
    QFileInfo finfo(path);

    if ( !options.format )
        options.format = io::glaxnimate::GlaxnimateFormat::instance();

    if ( url.isLocalFile() )
    {
        options.filename = path;
        options.path = finfo.absoluteDir();

        QFile file(options.filename);
        return d->do_save_document(document, options, file);
    }
    else
    {
        options.filename = url.toString();
        QByteArray data;
        QBuffer buf(&data);
        bool ok = d->do_save_document(document, options, buf);
        if ( !ok || !AndroidFilePicker::write_content_uri(url, data) )
        {
            QMessageBox::warning(d->widget_parent, QObject::tr("Save File"), QObject::tr("Could not save the file"));
            return false;
        }
        return true;
    }
}

std::unique_ptr<glaxnimate::model::Document> glaxnimate::android::DocumentOpener::open(const QUrl &url) const
{
    if ( !url.isValid() )
        return {};

    QString path = url.path();
    QFileInfo finfo(path);
    QString extension = finfo.suffix();
    io::Options options;
    options.format = io::IoRegistry::instance().from_extension(extension, io::ImportExport::Import);

    if ( url.isLocalFile() )
    {
        if ( !options.format )
        {
            QMessageBox::warning(d->widget_parent, QObject::tr("Open File"), QObject::tr("Unknown file type"));
            return {};
        }
        options.filename = path;
        options.path = finfo.absoluteDir();
        QFile file(options.filename);

        auto current_document = std::make_unique<model::Document>(options.filename);
        if ( !options.format->open(file, options.filename, current_document.get(), options.settings) )
            return {};

        current_document->set_io_options(options);
        return current_document;
    }

    options.filename = url.toString();
    QByteArray data = AndroidFilePicker::read_content_uri(url);
    bool zipped = false;
    if ( !options.format )
    {
        zipped = utils::gzip::is_compressed(data);
        if ( zipped )
        {
            QByteArray out;
            if ( !utils::gzip::decompress(data, out, {}) )
            {
                QMessageBox::warning(d->widget_parent, QObject::tr("Open File"), QObject::tr("Could not unzip the file"));
                return {};
            }
            data = std::move(out);
        }

        // json
        if ( data.startsWith('{') )
        {
            if ( data.contains("\"__type__\"") )
                options.format = io::glaxnimate::GlaxnimateFormat::instance();
            else
                options.format = io::IoRegistry::instance().from_slug("lottie");
        }
        else if ( data.contains("<svg") || data.contains("http://www.w3.org/2000/svg") )
        {
            options.format = io::IoRegistry::instance().from_slug("svg");
            options.settings["compressed"] = zipped;
        }
        else if ( data.startsWith("\x89PNG") )
        {
            return d->from_raster(data);
        }
    }

    if ( !options.format )
    {

        QString supported_formats;
        for ( const auto& fmt : io::IoRegistry::instance().importers() )
        {
            if ( fmt->slug() == "raster" )
                continue;
            if ( !supported_formats.isEmpty() )
                supported_formats += ",\n";
            supported_formats += fmt->name();
        }
        supported_formats += ",\nPNG";

        QMessageBox::warning(d->widget_parent, QObject::tr("Open File"), QObject::tr("Unknown file type. Supported files:\n%1").arg(supported_formats));
        return {};
    }

    QBuffer file(&data);
    auto current_document = std::make_unique<model::Document>(options.filename);
    if ( !options.format->open(file, options.filename, current_document.get(), options.settings) )
    {
        QMessageBox::warning(d->widget_parent, QObject::tr("Open File"), QObject::tr("Error loading %1 file").arg(options.format->slug()));
        return {};
    }

    if ( zipped && options.format->slug() == "lottie" )
        options.format = io::IoRegistry::instance().from_slug("tgs");

    current_document->set_io_options(options);
    return current_document;
}

std::unique_ptr<glaxnimate::model::Document> glaxnimate::android::DocumentOpener::from_raster(const QByteArray &data)
{
    return d->from_raster(data);
}
