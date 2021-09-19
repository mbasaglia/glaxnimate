#include "document.hpp"

#include <QRegularExpression>
#include <QPainter>

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "model/assets/assets.hpp"


class glaxnimate::model::Document::Private
{
public:
    using NameIndex = unsigned long long;

    Private(Document* doc)
        : main(doc),
          assets(doc)
    {
        io_options.format = io::glaxnimate::GlaxnimateFormat::instance();
    }

    std::pair<QString, NameIndex> name_index(const QString& name) const
    {
        static QRegularExpression detect_numbers("^(.*) ([0-9]+)$");
        QRegularExpressionMatch match = detect_numbers.match(name);
        if ( match.hasMatch() )
        {
            return {match.captured(1), match.captured(2).toULongLong()};
        }

        return {name, 0};
    }

    void increase(std::pair<QString, NameIndex> pair)
    {
        auto iter = name_indices.find(pair.first);

        if ( iter != name_indices.end() )
        {
            if ( iter->second < pair.second )
                iter->second = pair.second;
        }
        else
        {
            name_indices.emplace(std::move(pair));
        }
    }

    void decrease(const std::pair<QString, NameIndex>& pair)
    {
        if ( pair.second == 0 )
            return;

        auto iter = name_indices.find(pair.first);

        if ( iter != name_indices.end() )
        {
            if ( iter->second == pair.second )
                iter->second -= 1;
        }
    }

    QString name_suggestion(const QString& base_name)
    {
        auto index_pair = name_index(base_name);
        auto iter = name_indices.find(index_pair.first);
        if ( iter == name_indices.end() )
            return base_name;

        return QString("%1 %2").arg(iter->first).arg(iter->second + 1);
    }

    MainComposition main;
    QUndoStack undo_stack;
    QVariantMap metadata;
    io::Options io_options;
    FrameTime current_time = 0;
    bool record_to_keyframe = false;
    Assets assets;
    glaxnimate::model::CompGraph comp_graph;
    std::unordered_map<QString, NameIndex> name_indices;
};


glaxnimate::model::Document::Document(const QString& filename)
    : d ( std::make_unique<glaxnimate::model::Document::Private>(this) )
{
    d->io_options.filename = filename;
    d->comp_graph.add_composition(&d->main);
}

glaxnimate::model::Document::~Document() = default;

QString glaxnimate::model::Document::filename() const
{
    return d->io_options.filename;
}

glaxnimate::model::MainComposition * glaxnimate::model::Document::main()
{
    return &d->main;
}

QVariantMap & glaxnimate::model::Document::metadata()
{
    return d->metadata;
}

QUndoStack & glaxnimate::model::Document::undo_stack()
{
    return d->undo_stack;
}

const glaxnimate::io::Options & glaxnimate::model::Document::io_options() const
{
    return d->io_options;
}

void glaxnimate::model::Document::set_io_options(const io::Options& opt)
{
    bool em = opt.filename != d->io_options.filename;
    d->io_options = opt;
    if ( em )
        emit filename_changed(d->io_options.filename);
}

glaxnimate::model::DocumentNode * glaxnimate::model::Document::find_by_uuid(const QUuid& n) const
{
    if ( auto it = d->assets.find_by_uuid(n) )
        return it;
    return d->main.docnode_find_by_uuid(n);
}

glaxnimate::model::DocumentNode * glaxnimate::model::Document::find_by_name(const QString& name) const
{
    return d->main.docnode_find_by_name(name);
}

QVariantList glaxnimate::model::Document::find_by_type_name(const QString& type_name) const
{
    return d->main.find_by_type_name(type_name);
}

bool glaxnimate::model::Document::redo()
{
    if ( ! d->undo_stack.canRedo() )
        return false;
    d->undo_stack.redo();
    return true;
}

bool glaxnimate::model::Document::undo()
{
    if ( ! d->undo_stack.canUndo() )
        return false;
    d->undo_stack.undo();
    return true;
}

glaxnimate::model::FrameTime glaxnimate::model::Document::current_time() const
{
    return d->current_time;
}

void glaxnimate::model::Document::set_current_time(glaxnimate::model::FrameTime t)
{
    if ( t >= 0 && t <= d->main.animation->last_frame.get() )
    {
        d->main.set_time(t);
        d->assets.set_time(t);
        emit current_time_changed(d->current_time = t);
    }
}

QSize glaxnimate::model::Document::size() const
{
    return {
        d->main.width.get(),
        d->main.height.get()
    };
}

bool glaxnimate::model::Document::record_to_keyframe() const
{
    return d->record_to_keyframe;
}

void glaxnimate::model::Document::set_record_to_keyframe(bool r)
{
    emit(record_to_keyframe_changed(d->record_to_keyframe = r));
}

void glaxnimate::model::Document::push_command(QUndoCommand* cmd)
{
    d->undo_stack.push(cmd);
}

QString glaxnimate::model::Document::get_best_name(glaxnimate::model::DocumentNode* node, const QString& suggestion) const
{
    if ( !node )
        return {};

    if ( suggestion.isEmpty() )
        return d->name_suggestion(node->type_name_human());

    return d->name_suggestion(suggestion);

}

void glaxnimate::model::Document::set_best_name(glaxnimate::model::DocumentNode* node, const QString& suggestion) const
{
    if ( node )
        node->name.set(get_best_name(node, suggestion));
}

QRectF glaxnimate::model::Document::rect() const
{
    return QRectF(QPointF(0, 0), size());
}

glaxnimate::model::Assets * glaxnimate::model::Document::assets() const
{
    return &d->assets;
}

glaxnimate::model::Object * glaxnimate::model::Document::assets_obj() const
{
    return assets();
}

QImage glaxnimate::model::Document::render_image(float time, QSize image_size, const QColor& background) const
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
    d->main.paint(&painter, time, VisualNode::Render);

    return image;
}

QImage glaxnimate::model::Document::render_image() const
{
    return render_image(d->current_time, size());
}

void glaxnimate::model::Document::set_metadata(const QVariantMap& meta)
{
    d->metadata = meta;
}

glaxnimate::model::CompGraph & glaxnimate::model::Document::comp_graph()
{
    return d->comp_graph;
}

void glaxnimate::model::Document::decrease_node_name(const QString& old_name)
{
    if ( !old_name.isEmpty() )
        d->decrease(d->name_index(old_name));
}

void glaxnimate::model::Document::increase_node_name(const QString& new_name)
{
    if ( !new_name.isEmpty() )
        d->increase(d->name_index(new_name));
}


void glaxnimate::model::Document::stretch_time(qreal multiplier)
{
    qreal time = d->current_time;
    d->main.stretch_time(multiplier);
    d->assets.stretch_time(multiplier);
    set_current_time(qRound(time * multiplier));
}
