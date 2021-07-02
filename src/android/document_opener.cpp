#include "document_opener.hpp"

#include <QMessageBox>

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "utils/gzip.hpp"

#include "android_file_picker.hpp"


class glaxnimate::android::DocumentOpener::Private
{
public:
    QWidget* widget_parent;

    bool do_save_document(model::Document *document, const io::Options& opts, QIODevice& file)
    {
        return opts.format->save(file, opts.filename, document, opts.settings);
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

std::unique_ptr<model::Document> glaxnimate::android::DocumentOpener::open(const QUrl &url) const
{
    if ( !url.isValid() )
        return {};

    QString path = url.path();
    QFileInfo finfo(path);
    QString extension = finfo.suffix();
    io::Options options;
    options.format = io::IoRegistry::instance().from_extension(extension);

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
        else if ( data.contains("<svg>") )
        {
            options.format = io::IoRegistry::instance().from_slug("lottie");
            options.settings["compressed"] = true;
        }
    }

    if ( !options.format )
    {
        QMessageBox::warning(d->widget_parent, QObject::tr("Open File"), QObject::tr("Unknown file type"));
        return {};
    }

    QBuffer file(&data);
    auto current_document = std::make_unique<model::Document>(options.filename);
    if ( !options.format->open(file, options.filename, current_document.get(), options.settings) )
        return {};

    if ( zipped && options.format->slug() == "lottie" )
        options.format = io::IoRegistry::instance().from_slug("tgs");

    current_document->set_io_options(options);
    return current_document;
}
