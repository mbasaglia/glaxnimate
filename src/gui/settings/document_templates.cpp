#include "document_templates.hpp"

#include <QAction>

#include "app/application.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "model/assets/assets.hpp"


settings::DocumentTemplate::DocumentTemplate(const QString& filename, bool* loaded)
: filename(filename), document(load(loaded))
{}

QSize settings::DocumentTemplate::size() const
{
    return document->size();
}

model::FrameTime settings::DocumentTemplate::duration() const
{
    return document->main()->animation->last_frame.get() - document->main()->animation->first_frame.get();
}

QString settings::DocumentTemplate::name() const
{
    return name_template(document.get()).arg(document->main()->name.get());
}

float settings::DocumentTemplate::fps() const
{
    return document->main()->fps.get();
}

std::unique_ptr<model::Document> settings::DocumentTemplate::create(bool* ok) const
{
    std::unique_ptr<model::Document> document;
    document = load(ok);
    document->main()->refresh_uuid();
    document->assets()->refresh_uuid();
    document->set_best_name(document->main());
    return document;
}

std::unique_ptr<model::Document> settings::DocumentTemplate::load(bool* ok) const
{
    std::unique_ptr<model::Document> document = std::make_unique<model::Document>("");
    QFile file(filename);
    *ok = io::glaxnimate::GlaxnimateFormat().open(file, filename, document.get(), {});
    return document;
}

bool settings::DocumentTemplate::operator<(const settings::DocumentTemplate& oth) const
{
    return name() < oth.name();
}

QString settings::DocumentTemplate::name_template(model::Document* document)
{
    //: %4 is the file name, %1x%2 is the size, %3 is the frame rate
    return DocumentTemplates::tr("%4 (%1x%2 %3 fps)")
        .arg(document->main()->width.get())
        .arg(document->main()->height.get())
        .arg(document->main()->fps.get())
    ;
}

QAction* settings::DocumentTemplates::create_action(const DocumentTemplate& templ, QObject *parent)
{
    QAction* action = new QAction(QIcon::fromTheme("document-new-from-template"), templ.name(), parent);
    connect(action, &QAction::triggered, this, [&templ, this]{
        emit create_from(templ);
    });
    return action;
}

void settings::DocumentTemplates::load()
{
    templates_.clear();
    names.clear();

    for ( QDir dir : app::Application::instance()->data_paths("templates") )
    {
        for ( const auto& filename : dir.entryList(QDir::Files|QDir::Readable) )
        {
            bool ok = false;
            DocumentTemplate templ(dir.absoluteFilePath(filename), &ok);
            if ( ok && !names.count(templ.name()) )
            {
                names.insert(templ.name());
                templates_.push_back(std::move(templ));
            }
        }
    }

    std::sort(templates_.begin(), templates_.end());

    emit loaded(templates_);
}

settings::DocumentTemplates::DocumentTemplates()
{
    load();
}

bool settings::DocumentTemplates::save_as_template(model::Document* document)
{
    QString name_base = document->main()->name.get();
    if ( name_base.isEmpty() )
        name_base = tr("Template");

    // Find a nice name, avoiding duplicates
    QString name_full_template = DocumentTemplate::name_template(document);
    QString name_template = tr("%1 %2").arg(name_base);
    QString name = name_base;
    QString name_full = name_full_template.arg(name);
    for ( int count = 1; names.count(name_full); count++ )
    {
        name = name_template.arg(count);
        name_full = name_full_template.arg(name);
    }

    // Generate a file name
    QString basename;
    for ( const QChar& ch : name )
    {
        if ( ch.isLetterOrNumber() || ch == '_' || ch == '-' )
            basename += ch;
        else
            basename += '_';
    }

    io::glaxnimate::GlaxnimateFormat format;
    QDir dirname = app::Application::instance()->writable_data_path("templates");
    QString path = dirname.absoluteFilePath(basename + "." + format.extensions()[0]);

    if ( !dirname.exists() )
    {
        dirname.cdUp();
        dirname.mkpath("templates");
    }

    QFile file(path);
    if ( !format.save(file, path, document, {}) )
        return false;
    file.close();

    bool ok = false;
    DocumentTemplate templ(path, &ok);
    if ( !ok )
        return false;

    auto it = std::lower_bound(templates_.begin(), templates_.end(), templ);
    templates_.insert(it, std::move(templ));
    names.insert(name_full);

    emit loaded(templates_);

    return true;
}

settings::DocumentTemplates & settings::DocumentTemplates::instance()
{
    static settings::DocumentTemplates instance;
    return instance;
}

const std::vector<settings::DocumentTemplate> & settings::DocumentTemplates::templates() const
{
    return templates_;
}
