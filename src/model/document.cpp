#include "document.hpp"
#include "moc_layers.cpp" // HACK to avoid having layers.cpp

class model::Document::Private
{
public:
    Animation animation;
    QUndoStack undo_stack;
    QString source_filename;
    QDir save_path;
    QVariantMap metadata;
};


model::Document::Document()
    : d ( std::make_unique<model::Document::Private>() )
{
}

model::Document::~Document() = default;

QDir model::Document::save_path() const
{
    return d->save_path;
}

void model::Document::set_save_path(const QDir& p)
{
    d->save_path = p;
}

QString model::Document::source_filename() const
{
    return d->source_filename;
}

void model::Document::set_source_filename(const QString& n)
{
    d->source_filename = n;
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
