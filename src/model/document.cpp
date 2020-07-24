#include "document.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"


class model::Document::Private
{
public:
    Private(Document* doc)
        : animation(doc)
    {
        io_options.format = io::glaxnimate::GlaxnimateFormat::registered();
    }

    Animation animation;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options io_options;
    QUuid uuid = QUuid::createUuid();
    QString uuid_string = uuid.toString();
    int id = 0;
};


model::Document::Document(const QString& filename)
    : d ( std::make_unique<model::Document::Private>(this) )
{
    d->io_options.filename = filename;
}

model::Document::~Document() = default;

QString model::Document::filename() const
{
    return d->io_options.filename;
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

const io::Options & model::Document::io_options() const
{
    return d->io_options;
}

void model::Document::set_io_options(const io::Options& opt)
{
    bool em = opt.filename != d->io_options.filename;
    d->io_options = opt;
    if ( em )
        emit filename_changed(d->io_options.filename);
}

model::DocumentNode * model::Document::node_by_uuid(const QUuid& n) const
{
    return d->animation.docnode_find_by_uuid(n);
}
