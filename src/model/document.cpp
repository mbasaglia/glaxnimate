#include "document.hpp"
#include "moc_layers.cpp" // HACK to avoid having layers.cpp

class model::Document::Private
{
public:
    Animation animation;
    QUndoStack undo_stack;
    QString filename;
    QDir save_path;
    QVariantMap metadata;
};


model::Document::Document(const QString& filename)
    : d ( std::make_unique<model::Document::Private>() )
{
    d->filename = filename;
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

QString model::Document::filename() const
{
    return d->filename;
}

void model::Document::set_filename(const QString& n)
{
    d->filename = n;
    emit filename_changed(n);
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
