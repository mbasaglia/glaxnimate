#include "glaxnimate_window_p.hpp"

#include <queue>

#include <QClipboard>
#include <QImageReader>
#include <QFileDialog>

#include "app/settings/widget_builder.hpp"

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "model/shapes/image.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/simple_visitor.hpp"

#include "settings/clipboard_settings.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"
#include "widgets/tab_bar_close_button.hpp"


model::Composition* GlaxnimateWindow::Private::current_composition()
{
    return comp;
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
    if ( auto dn = document_node_model.node(comp_model.mapToSource(ui.view_document_node->currentIndex())) )
        return dn;
    return current_document->main();
}

void GlaxnimateWindow::Private::set_current_document_node(model::DocumentNode* node)
{
    ui.view_document_node->setCurrentIndex(comp_model.mapFromSource(document_node_model.node_index(node)));
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

    ui.view_document_node->setCurrentIndex(comp_model.mapFromSource(document_node_model.node_index(ptr)));
}

void GlaxnimateWindow::Private::layer_delete()
{
    auto current = current_shape();
    if ( !current || current->locked.get() )
        return;
    current->push_command(new command::RemoveShape(current, current->owner()));
}

void GlaxnimateWindow::Private::layer_duplicate()
{
    auto current = current_shape();
    if ( !current )
        return;

    auto cmd = command::duplicate_shape(current);
    current->push_command(cmd);
    set_current_document_node(cmd->object());
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

    command::UndoMacroGuard macro(tr("Delete"), current_document.get());
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->locked.get() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
}

void GlaxnimateWindow::Private::cut()
{
    auto selection = copy();
    if ( selection.empty() )
        return;

    command::UndoMacroGuard macro(tr("Cut"), current_document.get());
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->locked.get() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
}

std::vector<model::DocumentNode*> GlaxnimateWindow::Private::copy()
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& mime : settings::ClipboardSettings::mime_types() )
        {
            if ( mime.enabled )
                mime.serializer->to_mime_data(*data, selection);
        }

        QGuiApplication::clipboard()->setMimeData(data);
    }

    return selection;
}

template<class T>
static void paste_assets(model::ObjectListProperty<T> (model::Defs::* prop), model::Document* source, model::Document* current_document)
{
    for ( auto& item : (source->defs()->*prop).raw() )
    {
        if ( !current_document->defs()->find_by_uuid(item->uuid.get()) )
        {
            item->transfer(current_document);
            current_document->push_command(new command::AddObject(
                &(current_document->defs()->*prop),
                std::move(item),
                (current_document->defs()->*prop).size()
            ));
        }
    }
}

void GlaxnimateWindow::Private::paste()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    io::mime::DeserializedData raw_pasted;
    for ( const auto& mime : settings::ClipboardSettings::mime_types() )
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

    command::UndoMacroGuard macro(tr("Paste"), current_document.get());
    paste_assets(&model::Defs::colors, raw_pasted.document.get(), current_document.get());
    paste_assets(&model::Defs::images, raw_pasted.document.get(), current_document.get());
    paste_assets(&model::Defs::gradient_colors, raw_pasted.document.get(), current_document.get());
    paste_assets(&model::Defs::gradients, raw_pasted.document.get(), current_document.get());
    paste_assets(&model::Defs::precompositions, raw_pasted.document.get(), current_document.get());

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

    QItemSelection item_select;
    for ( auto node : select )
    {
        item_select.push_back(QItemSelectionRange(comp_model.mapFromSource(document_node_model.node_index(node))));
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
        command::UndoMacroGuard macro(tr("Move Shapes"), current_document.get());
        for ( auto shape : shapes )
            if ( shape->owner() != parent )
                shape->push_command(new command::MoveShape(shape, shape->owner(), parent, parent->size()));
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

    command::UndoMacroGuard macro(tr("Import Image"), current_document.get());

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
    set_current_document_node(select);
}

void GlaxnimateWindow::Private::document_treeview_current_changed(const QModelIndex& index)
{
    model::Stroke* stroke = nullptr;
    model::Fill* fill = nullptr;
    if ( auto node = document_node_model.node(comp_model.mapToSource(index)) )
    {
        property_model.set_object(node);
        ui.timeline_widget->set_active(node);
        ui.view_properties->expandAll();

        stroke = qobject_cast<model::Stroke*>(node);
        fill = qobject_cast<model::Fill*>(node);
        if ( !stroke && !fill )
        {
            auto group = qobject_cast<model::Group*>(node);

            if ( !group )
            {
                if ( auto parent = node->docnode_parent() )
                    group = qobject_cast<model::Group*>(parent);
            }

            if ( group )
            {
                int stroke_count = 0;
                int fill_count = 0;
                for ( const auto& shape : group->shapes )
                {
                    if ( auto s = qobject_cast<model::Stroke*>(shape.get()) )
                    {
                        stroke = s;
                        stroke_count++;
                    }
                    else if ( auto f = qobject_cast<model::Fill*>(shape.get()) )
                    {
                        fill = f;
                        fill_count++;
                    }
                }

                if ( stroke_count > 1 )
                    stroke = nullptr;

                if ( fill_count > 1 )
                    fill = nullptr;
            }
        }
    }
    else
    {
        property_model.set_object(nullptr);
        ui.timeline_widget->set_active(nullptr);
    }

    ui.stroke_style_widget->set_shape(stroke);
    ui.fill_style_widget->set_shape(fill);
    ui.widget_gradients->set_targets(fill, stroke);
    if ( fill )
        set_brush_reference(fill->use.get(), false);
    if ( stroke )
        set_brush_reference(stroke->use.get(), true);
}

template<class T>
static void remove_assets(T& prop, int& count)
{
    for ( int i = 0; i < prop.size();  )
    {
        if ( prop[i]->remove_if_unused(true) )
            count++;
        else
            i++;
    }
}

void GlaxnimateWindow::Private::cleanup_document()
{
    command::UndoMacroGuard guard(tr("Cleanup Document"), current_document.get());
    int count = 0;

    remove_assets(current_document->defs()->gradients, count);
    remove_assets(current_document->defs()->gradient_colors, count);
    remove_assets(current_document->defs()->colors, count);
    remove_assets(current_document->defs()->images, count);

    status_message(tr("Removed %1 assets").arg(count), 0);
}

void GlaxnimateWindow::Private::to_path()
{
    std::set<model::Shape*> shapes;

    auto callback = [&shapes](model::Shape* shape){ shapes.insert(shape); };
    for ( auto selected : scene.selection() )
        model::simple_visit<model::Shape>(selected, true, callback);

    if ( shapes.empty() )
        return;

    QString macro_name = tr("Convert to path");
    if ( shapes.size() == 1 )
        macro_name = tr("Convert %1 to path").arg((*shapes.begin())->name.get());

    command::UndoMacroGuard guard(macro_name, current_document.get());
    for ( auto shape : shapes )
    {
        current_document->push_command(
            new command::AddObject<model::ShapeElement>(
                shape->owner(),
                shape->to_path(),
                shape->position()
            )
        );
        current_document->push_command(
            new command::RemoveObject<model::ShapeElement>(shape, shape->owner())
        );
    }
}

void GlaxnimateWindow::Private::switch_composition(int i)
{
    int old_i = current_document->defs()->precompositions.index_of(static_cast<model::Precomposition*>(comp)) + 1;
    comp_selections[old_i].selection = scene.selection();
    if ( ui.view_document_node->currentIndex().isValid() )
        comp_selections[old_i].current = document_node_model.node(comp_model.mapToSource(ui.view_document_node->currentIndex()));
    else
        comp_selections[old_i].current = comp;

    if ( old_i > 0 )
        ui.menu_new_comp_layer->actions()[old_i-1]->setEnabled(true);

    int precomp_index = i - 1;
    if ( precomp_index >= 0 && precomp_index < current_document->defs()->precompositions.size() )
    {
        comp = current_document->defs()->precompositions[precomp_index];
        ui.menu_new_comp_layer->actions()[precomp_index]->setEnabled(false);
    }
    else
    {
        comp = current_document->main();
    }


    comp_model.set_composition(comp);
    scene.set_composition(comp);
    scene.user_select(comp_selections[i].selection, graphics::DocumentScene::Replace);
    auto current = comp_selections[i].current;
    ui.view_document_node->setCurrentIndex(comp_model.mapFromSource(document_node_model.node_index(current)));
}

void GlaxnimateWindow::Private::setup_composition(model::Composition* comp, int index)
{
    index = ui.tab_bar->insertTab(index, comp->docnode_icon(), comp->object_name());
    CompState state;
    if ( !comp->shapes.empty() )
        state = comp->shapes[0];
    else
        state = comp;

    comp_selections.insert(comp_selections.begin() + index, std::move(state));
    update_comp_color(index, comp);
    QAction* action = nullptr;

    if ( comp != current_document->main() )
    {
        ui.menu_new_comp_layer->setEnabled(true);
        action = new QAction(comp->reftarget_icon(), comp->object_name(), comp);
        if ( ui.menu_new_comp_layer->actions().empty() || index - 1 >= ui.menu_new_comp_layer->actions().size() )
            ui.menu_new_comp_layer->addAction(action);
        else
            ui.menu_new_comp_layer->insertAction(ui.menu_new_comp_layer->actions()[index-1], action);
        action->setData(QVariant::fromValue(comp));

        TabBarCloseButton::add_button(ui.tab_bar, index);
    }

    connect(comp, &model::ReferenceTarget::name_changed, ui.tab_bar, [this, index, comp, action](){
        ui.tab_bar->setTabText(index, comp->object_name());
        if ( action )
            action->setText(comp->object_name());
    });
    connect(comp, &model::DocumentNode::docnode_group_color_changed, ui.tab_bar, [this, index, comp, action](){
        update_comp_color(index, comp);
        if ( action )
            action->setIcon(comp->reftarget_icon());
    });

}

void GlaxnimateWindow::Private::add_composition()
{
    std::unique_ptr<model::Precomposition> comp = std::make_unique<model::Precomposition>(current_document.get());

    auto lay = std::make_unique<model::Layer>(current_document.get());
    current_document->set_best_name(lay.get());
    auto center = current_document->rect().center();
    lay->transform->anchor_point.set(center);
    lay->transform->position.set(center);
    comp->shapes.insert(std::move(lay));

    current_document->set_best_name(comp.get());
    current_document->push_command(new command::AddObject(&current_document->defs()->precompositions, std::move(comp)));
    ui.tab_bar->setCurrentIndex(ui.tab_bar->count()-1);
}

void GlaxnimateWindow::Private::objects_to_new_composition(
    model::Composition* comp,
    const std::vector<model::DocumentNode*>& objects,
    model::ObjectListProperty<model::ShapeElement>* layer_parent,
    int layer_index
)
{
    if ( objects.empty() )
        return;

    command::UndoMacroGuard guard(tr("New Composition from Selection"), current_document.get());

    std::unique_ptr<model::Precomposition> ucomp = std::make_unique<model::Precomposition>(current_document.get());
    model::Precomposition* new_comp = ucomp.get();
    current_document->set_best_name(new_comp);
    current_document->push_command(new command::AddObject(&current_document->defs()->precompositions, std::move(ucomp)));


    for ( auto node : objects )
    {
        if ( auto shape = node->cast<model::ShapeElement>() )
            current_document->push_command(new command::MoveShape(
                shape, shape->owner(), &new_comp->shapes, new_comp->shapes.size()
            ));
    }

    comp_selections.back().current = objects[0];
    comp_selections.back().selection = objects;

    auto pcl = std::make_unique<model::PreCompLayer>(current_document.get());
    pcl->composition.set(new_comp);
    pcl->size.set(current_document->size());
    current_document->set_best_name(pcl.get());
    auto pcl_ptr = pcl.get();
    current_document->push_command(new command::AddShape(layer_parent, std::move(pcl), layer_index));

    ui.tab_bar->setCurrentIndex(ui.tab_bar->count()-1);

    int old_comp_index = current_document->defs()->precompositions.index_of(static_cast<model::Precomposition*>(comp)) + 1;
    comp_selections[old_comp_index] = pcl_ptr;
}

void GlaxnimateWindow::Private::update_comp_color(int index, model::Composition* comp)
{
    QColor c = comp->group_color.get();
    if ( c.alpha() == 0 )
        c = parent->palette().text().color();
    else
        c.setAlpha(255);
    ui.tab_bar->setTabTextColor(index, c);
}

void GlaxnimateWindow::Private::on_remove_precomp(int index)
{
    model::Precomposition* precomp = current_document->defs()->precompositions[index];
    if ( precomp == comp )
        switch_composition(0);

    ui.tab_bar->removeTab(index+1);
    delete ui.menu_new_comp_layer->actions()[index];
    comp_selections.erase(comp_selections.begin()+index+1);
}


void GlaxnimateWindow::Private::layer_new_comp(QAction* action)
{
    auto layer = std::make_unique<model::PreCompLayer>(current_document.get());
    layer->animation->last_frame.set(current_document->main()->animation->last_frame.get());
    layer->composition.set(action->data().value<model::Precomposition*>());
    layer->size.set(current_document->rect().size());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer_new_impl(std::move(layer));
}


void GlaxnimateWindow::Private::composition_close_request(int index)
{
    if ( index > 0 )
    {
        current_document->push_command(new command::RemoveObject<model::Precomposition>(
            index-1, &current_document->defs()->precompositions
        ));
    }
}

void GlaxnimateWindow::Private::shape_to_precomposition(model::ShapeElement* node)
{
    if ( !node )
        return;

    auto parent = node->docnode_parent();
    if ( !parent )
        return;

    auto ancestor = parent;
    auto grand_ancestor = ancestor->docnode_parent();
    while ( grand_ancestor )
    {
        ancestor = grand_ancestor;
        grand_ancestor = ancestor->docnode_parent();
    }

    auto owner_comp = ancestor->cast<model::Composition>();
    if ( !owner_comp )
        return;

    auto prop = parent->get_property("shapes");
    if ( !prop )
        return;

    auto shape_prop = static_cast<model::ObjectListProperty<model::ShapeElement>*>(prop);
    objects_to_new_composition(owner_comp, {node}, shape_prop, shape_prop->index_of(node));
}
