#include "glaxnimate_window_p.hpp"

#include <queue>

#include <QClipboard>
#include <QImageReader>
#include <QFileDialog>

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "app/settings/widget_builder.hpp"
#include "model/shapes/group.hpp"
#include "misc/clipboard_settings.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"
#include "model/shapes/image.hpp"


model::Composition* GlaxnimateWindow::Private::current_composition()
{
    return current_document->main();
}

model::ShapeElement* GlaxnimateWindow::Private::current_shape()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::ShapeListProperty* GlaxnimateWindow::Private::current_shape_container()
{
    model::DocumentNode* sh = current_document_node();
    if ( auto lay = qobject_cast<model::Composition*>(sh) )
        return &lay->shapes;

    if ( !qobject_cast<model::Layer*>(sh) )
        sh = sh->docnode_parent();

    while ( sh )
    {
        if ( auto grp = qobject_cast<model::Group*>(sh) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::Composition*>(sh) )
            return &lay->shapes;
        sh = sh->docnode_parent();
    }
    return &current_composition()->shapes;
}

model::DocumentNode* GlaxnimateWindow::Private::current_document_node()
{
    if ( auto dn = document_node_model.node(ui.view_document_node->currentIndex()) )
        return dn;
    return current_document->main();
}

void GlaxnimateWindow::Private::set_current_document_node(model::DocumentNode* node)
{
    ui.view_document_node->setCurrentIndex(document_node_model.node_index(node));
}


void GlaxnimateWindow::Private::layer_new_layer()
{
    auto layer = std::make_unique<model::Layer>(current_document.get());
    layer->animation->last_frame.set(current_document->main()->animation->last_frame.get());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_fill()
{
    auto layer = std::make_unique<model::Fill>(current_document.get());
    layer->color.set(ui.fill_style_widget->current_color());
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_stroke()
{
    auto layer = std::make_unique<model::Stroke>(current_document.get());
    layer->set_pen_style(ui.stroke_style_widget->pen_style());
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_group()
{
    auto layer = std::make_unique<model::Group>(current_document.get());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_impl(std::unique_ptr<model::ShapeElement> layer)
{
    current_document->set_best_name(layer.get(), {});
    layer->set_time(current_document_node()->time());

    model::ShapeElement* ptr = layer.get();

    auto cont = current_shape_container();
    int position = cont->index_of(current_shape());
    current_document->push_command(new command::AddShape(cont, std::move(layer), position));

    ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
}

void GlaxnimateWindow::Private::layer_delete()
{
    auto current = current_shape();
    if ( !current || current->docnode_locked() )
        return;
    current->push_command(new command::RemoveShape(current, current->owner()));
}

void GlaxnimateWindow::Private::layer_duplicate()
{
    auto current = current_shape();
    if ( !current )
        return;
    current->push_command(command::duplicate_shape(current));
}

std::vector<model::DocumentNode *> GlaxnimateWindow::Private::cleaned_selection()
{
    return scene.cleaned_selection();
}


void GlaxnimateWindow::Private::delete_selected()
{
    auto selection = cleaned_selection();
    if ( selection.empty() )
        return;

    current_document->undo_stack().beginMacro(tr("Delete"));
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->docnode_locked() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
    current_document->undo_stack().endMacro();
}

void GlaxnimateWindow::Private::cut()
{
    auto selection = copy();
    if ( selection.empty() )
        return;

    current_document->undo_stack().beginMacro(tr("Cut"));
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->docnode_locked() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
    current_document->undo_stack().endMacro();
}

std::vector<model::DocumentNode*> GlaxnimateWindow::Private::copy()
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& mime : ClipboardSettings::mime_types() )
        {
            if ( mime.enabled )
                mime.serializer->to_mime_data(*data, selection);
        }

        QGuiApplication::clipboard()->setMimeData(data);
    }

    return selection;
}

void GlaxnimateWindow::Private::paste()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    io::mime::DeserializedData raw_pasted;
    for ( const auto& mime : ClipboardSettings::mime_types() )
    {
        if ( mime.enabled )
        {
            raw_pasted = mime.serializer->from_mime_data(*data);
            if ( !raw_pasted.empty() )
                break;
        }
    }
    if ( raw_pasted.empty() )
    {
        status_message(tr("Nothing to paste"));
        return;
    }

    /// \todo precompositions

    current_document->undo_stack().beginMacro(tr("Paste"));

    for ( auto& color : raw_pasted.document->defs()->colors.raw() )
    {
        if ( !current_document->defs()->find_by_uuid(color->uuid.get()) )
        {
            color->transfer(current_document.get());
            current_document->push_command(new command::AddObject(
                &current_document->defs()->colors,
                std::move(color),
                current_document->defs()->colors.size()
            ));
        }
    }

    for ( auto& image : raw_pasted.document->defs()->images.raw() )
    {
        if ( !current_document->defs()->find_by_uuid(image->uuid.get()) )
        {
            image->transfer(current_document.get());
            current_document->push_command(new command::AddObject(
                &current_document->defs()->images,
                std::move(image),
                current_document->defs()->images.size()
            ));
        }
    }

    model::ShapeListProperty* shape_cont = current_shape_container();
    std::vector<model::DocumentNode*> select;
    if ( !raw_pasted.document->main()->shapes.empty() )
    {
        int shape_insertion_point = shape_cont->size();
        for ( auto& shape : raw_pasted.document->main()->shapes.raw() )
        {
            auto ptr = shape.get();
            shape->refresh_uuid();
            select.push_back(ptr);
            shape->transfer(current_document.get());
            current_document->push_command(new command::AddShape(shape_cont, std::move(shape), shape_insertion_point++));
            ptr->recursive_rename();
        }
    }

    current_document->undo_stack().endMacro();

    QItemSelection item_select;
    for ( auto node : select )
    {
        item_select.push_back(QItemSelectionRange(document_node_model.node_index(node)));
    }
    ui.view_document_node->selectionModel()->select(item_select, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
}


void GlaxnimateWindow::Private::move_current(command::ReorderCommand::SpecialPosition pos)
{
    auto current = current_shape();
    if ( !current )
        return;
    auto cmd = std::make_unique<command::ReorderCommand>(current, pos);
    if ( !cmd->has_action() )
        return;
    current->push_command(cmd.release());
}

void GlaxnimateWindow::Private::group_shapes()
{
    auto data = command::GroupShapes::collect_shapes(cleaned_selection());
    if ( data.parent )
        current_document->push_command(
            new command::GroupShapes(data)
        );
}

void GlaxnimateWindow::Private::ungroup_shapes()
{
    model::Group* group = qobject_cast<model::Group*>(current_document_node());

    if ( !group )
    {
        auto sp = current_shape_container();
        if ( !sp )
            return;
        group = qobject_cast<model::Group*>(sp->object());
    }

    if ( group )
        current_document->push_command(new command::UngroupShapes(group));
}


void GlaxnimateWindow::Private::move_to()
{
    auto sel = cleaned_selection();
    std::vector<model::ShapeElement*> shapes;
    shapes.reserve(sel.size());
    for ( const auto& node : sel )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
            shapes.push_back(shape);
    }

    if ( shapes.empty() )
        return;


    if ( auto parent = ShapeParentDialog(&document_node_model, this->parent).get_shape_parent() )
    {
        current_document->undo_stack().beginMacro(tr("Move Shapes"));
        for ( auto shape : shapes )
            if ( shape->owner() != parent )
                shape->push_command(new command::MoveShape(shape, shape->owner(), parent, parent->size()));
        current_document->undo_stack().endMacro();
    }
}

QString GlaxnimateWindow::Private::get_open_image_file(const QString& title, const QString& dir)
{
    QFileDialog dialog(parent, title, dir);
    QStringList filters;
    for ( const auto& baf : QImageReader::supportedMimeTypes() )
        filters.push_back(QString(baf));
    dialog.setMimeTypeFilters(filters);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);

    if ( dialog.exec() == QDialog::Rejected )
        return {};

    return dialog.selectedFiles()[0];
}


void GlaxnimateWindow::Private::import_image()
{
    QString image_file = get_open_image_file(tr("Import Image"), current_document->io_options().path.absolutePath());
    if ( image_file.isEmpty() )
        return;

    auto bitmap = std::make_unique<model::Bitmap>(current_document.get());
    bitmap->filename.set(image_file);
    if ( bitmap->pixmap().isNull() )
    {
        show_warning(tr("Import Image"), tr("Could not import image"));
        return;
    }
    /// \todo dialog asking whether to embed

    current_document->undo_stack().beginMacro(tr("Import Image"));

    auto defs = current_document->defs();
    auto bmp_ptr = bitmap.get();
    current_document->push_command(new command::AddObject(&defs->images, std::move(bitmap), defs->images.size()));

    auto image = std::make_unique<model::Image>(current_document.get());
    image->image.set(bmp_ptr);
    QPointF p(bmp_ptr->pixmap().width() / 2.0, bmp_ptr->pixmap().height() / 2.0);
    image->transform->anchor_point.set(p);
    image->transform->position.set(p);
    auto comp = current_composition();
    auto select = image.get();
    current_document->push_command(new command::AddShape(&comp->shapes, std::move(image), comp->shapes.size()));
    current_document->undo_stack().endMacro();
    set_current_document_node(select);
}
