#include "document.hpp"

#include <QUuid>

#include "io/glaxnimate/glaxnimate_exporter.hpp"


class model::Document::Private
{
public:
    Private()
    {
        exporter.method = io::glaxnimate::GlaxnimateExporter::registered();
    }

    std::unique_ptr<Animation> animation;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options exporter;
    QUuid uuid = QUuid::createUuid();
    QString uuid_string = uuid.toString();
    int id = 0;
};


model::Document::Document(const QString& filename)
    : d ( std::make_unique<model::Document::Private>() )
{
    d->exporter.filename = filename;
    d->animation = std::make_unique<model::Animation>(this);
}

model::Document::~Document() = default;

QString model::Document::filename() const
{
    return d->exporter.filename;
}

model::Animation & model::Document::animation()
{
    return *d->animation;
}

QVariantMap & model::Document::metadata() const
{
    return d->metadata;
}

QUndoStack & model::Document::undo_stack()
{
    return d->undo_stack;
}

const io::Options & model::Document::export_options() const
{
    return d->exporter;
}

void model::Document::set_export_options(const io::Options& opt)
{
    bool em = opt.filename != d->exporter.filename;
    d->exporter = opt;
    if ( em )
        emit filename_changed(d->exporter.filename);
}

QString model::Document::generate_id()
{
    return d->uuid_string + QString::number(d->id++);
}

void model::Document::set_uuid ( const QUuid& uuid )
{
    d->uuid = uuid;
    d->uuid_string = d->uuid.toString();
}

const QUuid & model::Document::uuid() const
{
    return d->uuid;
}



