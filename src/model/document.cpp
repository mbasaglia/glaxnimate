#include "document.hpp"

class model::Document::Private
{
public:
    Private(Document* document) : animation(document) {}

    Animation animation;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options exporter;
};


model::Document::Document(const QString& filename)
    : d ( std::make_unique<model::Document::Private>(this) )
{
    d->exporter.filename = filename;
}

model::Document::~Document() = default;

QString model::Document::filename() const
{
    return d->exporter.filename;
}

model::Animation & model::Document::animation()
{
    return d->animation;
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
