#include "document.hpp"

#include <QRegularExpression>
#include <QPainter>

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "model/defs/defs.hpp"


class model::Document::Private
{
public:
    Private(Document* doc)
        : main(doc),
          defs(doc)
    {
        io_options.format = io::glaxnimate::GlaxnimateFormat::instance();
    }

    MainComposition main;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options io_options;
    QUuid uuid = QUuid::createUuid();
    QString uuid_string = uuid.toString();
    int id = 0;
    FrameTime current_time = 0;
    bool record_to_keyframe = false;
    Defs defs;
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

model::MainComposition * model::Document::main()
{
    return &d->main;
}

QVariantMap & model::Document::metadata()
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

model::ReferenceTarget * model::Document::find_by_uuid(const QUuid& n) const
{
    if ( auto it = d->defs.find_by_uuid(n) )
        return it;
    return d->main.docnode_find_by_uuid(n);
}

model::DocumentNode * model::Document::find_by_name(const QString& name) const
{
    return d->main.docnode_find_by_name(name);
}

QVariantList model::Document::find_by_type_name(const QString& type_name) const
{
    return d->main.find_by_type_name(type_name);
}

bool model::Document::redo()
{
    if ( ! d->undo_stack.canRedo() )
        return false;
    d->undo_stack.redo();
    return true;
}

bool model::Document::undo()
{
    if ( ! d->undo_stack.canUndo() )
        return false;
    d->undo_stack.undo();
    return true;
}

model::FrameTime model::Document::current_time() const
{
    return d->current_time;
}

void model::Document::set_current_time(model::FrameTime t)
{
    if ( t >= 0 && t <= d->main.animation->last_frame.get() )
    {
        d->main.set_time(t);
        d->defs.set_time(t);
        emit current_time_changed(d->current_time = t);
    }
}

QSize model::Document::size() const
{
    return {
        d->main.width.get(),
        d->main.height.get()
    };
}

bool model::Document::record_to_keyframe() const
{
    return d->record_to_keyframe;
}

void model::Document::set_record_to_keyframe(bool r)
{
    emit(record_to_keyframe_changed(d->record_to_keyframe = r));
}

void model::Document::push_command(QUndoCommand* cmd)
{
    d->undo_stack.push(cmd);
}


static void collect_names(const model::DocumentNode* node, const QString& prefix, QVector<QString>& out, const model::DocumentNode* target)
{
    if ( node != target && node->name.get().startsWith(prefix) )
        out.push_back(node->name.get());
    for ( int i = 0, e = node->docnode_child_count(); i < e; i++ )
        collect_names(node->docnode_child(i), prefix, out, target);
}

QString model::Document::get_best_name(const model::DocumentNode* node, const QString& suggestion) const
{
    if ( !node )
        return {};

    QVector<QString> names;

    int n = 0;
    QString base_name = suggestion;
    if ( base_name.isEmpty() )
    {
        base_name = node->type_name_human();
    }
    else
    {
        static QRegularExpression detect_numbers("^(.*) ([0-9]+)$");
        QRegularExpressionMatch match = detect_numbers.match(base_name);
        if ( match.hasMatch() )
        {
            base_name = match.captured(1);
            n = match.captured(2).toInt();
        }
    }

    QString name = base_name;

    collect_names(&d->main, base_name, names, node);
    for ( const auto& comp : d->defs.precompositions )
        collect_names(comp.get(), base_name, names, node);

    QString name_pattern = "%1 %2";
    while ( names.contains(name) )
    {
        n += 1;
        name = name_pattern.arg(base_name).arg(n);
    }

    return name;
}

void model::Document::set_best_name(model::DocumentNode* node, const QString& suggestion) const
{
    if ( node )
        node->name.set(get_best_name(node, suggestion));
}

QRectF model::Document::rect() const
{
    return QRectF(QPointF(0, 0), size());
}

model::Defs * model::Document::defs() const
{
    return &d->defs;
}

model::Object * model::Document::defs_obj() const
{
    return defs();
}

QImage model::Document::render_image(float time, QSize image_size, const QColor& background) const
{
    QSizeF real_size = size();
    if ( !image_size.isValid() )
        image_size = real_size.toSize();
    QImage image(image_size, QImage::Format_RGBA8888);
    if ( !background.isValid() )
        image.fill(Qt::transparent);
    else
        image.fill(background);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(
        image_size.width() / real_size.width(),
        image_size.height() / real_size.height()
    );
    d->main.paint(&painter, time, DocumentNode::Render);

    return image;
}

QImage model::Document::render_image() const
{
    return render_image(d->current_time, size());
}

void model::Document::set_metadata(const QVariantMap& meta)
{
    d->metadata = meta;
}
