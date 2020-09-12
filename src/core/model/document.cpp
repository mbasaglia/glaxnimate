#include "document.hpp"

#include <QRegularExpression>

#include "io/glaxnimate/glaxnimate_format.hpp"


class model::Document::Private
{
public:
    Private(Document* doc)
        : main_composition(doc)
    {
        io_options.format = io::glaxnimate::GlaxnimateFormat::instance();
    }

    MainComposition main_composition;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options io_options;
    QUuid uuid = QUuid::createUuid();
    QString uuid_string = uuid.toString();
    int id = 0;
    FrameTime current_time = 0;
    bool has_file = false;
    bool record_to_keyframe = false;
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

model::MainComposition * model::Document::main_composition()
{
    return &d->main_composition;
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

model::DocumentNode * model::Document::find_by_uuid(const QUuid& n) const
{
    return d->main_composition.docnode_find_by_uuid(n);
}

model::DocumentNode * model::Document::find_by_name(const QString& name) const
{
    return d->main_composition.docnode_find_by_name(name);
}

QVariantList model::Document::find_by_type_name(const QString& type_name) const
{
    return d->main_composition.find_by_type_name(type_name);
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
    if ( t >= 0 && t <= d->main_composition.last_frame.get() )
    {
        d->main_composition.set_time(t);
        emit current_time_changed(d->current_time = t);
    }
}

QSize model::Document::size() const
{
    return {
        d->main_composition.width.get(),
        d->main_composition.height.get()
    };
}

bool model::Document::has_file() const
{
    return d->has_file;
}

void model::Document::set_has_file(bool hf)
{
    d->has_file = hf;
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


static void collect_names(const model::DocumentNode* node, const QString& prefix, QVector<QString>& out)
{
    if ( node->name.get().startsWith(prefix) )
        out.push_back(node->name.get());
    for ( int i = 0, e = node->docnode_child_count(); i < e; i++ )
        collect_names(node->docnode_child(i), prefix, out);
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

    /// \todo Also collect for precompositions
    collect_names(&d->main_composition, base_name, names);

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
