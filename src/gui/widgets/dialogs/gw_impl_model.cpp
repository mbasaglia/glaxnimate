/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <queue>

#include <QClipboard>
#include <QImageReader>
#include <QFileDialog>
#include <QMimeData>
#include <QMimeType>
#include <QMimeDatabase>

#include "app/settings/widget_builder.hpp"

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "model/shapes/image.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/simple_visitor.hpp"
#include "model/shapes/text.hpp"

#include "settings/clipboard_settings.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"

#include "item_models/drag_data.hpp"

model::Composition* GlaxnimateWindow::Private::current_composition()
{
    return comp;
}

model::VisualNode* GlaxnimateWindow::Private::current_document_node()
{
    if ( auto dn = ui.view_document_node->current_node() )
        return dn;
    return comp;
}

void GlaxnimateWindow::Private::layer_new_layer()
{
    auto layer = std::make_unique<model::Layer>(current_document.get());
    layer->animation->last_frame.set(comp->animation->last_frame.get());
    QPointF pos = comp->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    parent->layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_fill()
{
    auto layer = std::make_unique<model::Fill>(current_document.get());
    layer->color.set(ui.fill_style_widget->current_color());
    parent->layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_stroke()
{
    auto layer = std::make_unique<model::Stroke>(current_document.get());
    layer->set_pen_style(ui.stroke_style_widget->pen_style());
    parent->layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_group()
{
    auto layer = std::make_unique<model::Group>(current_document.get());
    QPointF pos = comp->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    parent->layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_delete()
{
    auto current = parent->current_shape();
    if ( !current )
        return;
    parent->delete_shapes_impl(tr("Delete %1").arg(current->object_name()), {current});
}

void GlaxnimateWindow::Private::layer_duplicate()
{
    auto current = parent->current_shape();
    if ( !current )
        return;

    auto cmd = command::duplicate_shape(current);
    current->push_command(cmd);
    set_current_document_node(cmd->object());
}

std::vector<model::VisualNode*> GlaxnimateWindow::Private::cleaned_selection()
{
    return scene.cleaned_selection();
}


void GlaxnimateWindow::Private::duplicate_selection()
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        std::vector<model::VisualNode*> duplicated;
        duplicated.reserve(selection.size());

        for ( const auto& node : selection )
        {
            if ( auto shape = node->cast<model::ShapeElement>() )
            {
                auto cmd = command::duplicate_shape(shape);
                current_document->push_command(cmd);
                duplicated.push_back(cmd->object());
            }
        }

        scene.user_select(duplicated, graphics::DocumentScene::Replace);
    }
}


void GlaxnimateWindow::Private::move_current(command::ReorderCommand::SpecialPosition pos)
{
    auto current = parent->current_shape();
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
        auto sp = parent->current_shape_container();
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

QStringList GlaxnimateWindow::Private::get_open_image_files(const QString& title, const QString& dir, QString* out_dir, bool multiple)
{
    QFileDialog dialog(parent, title, dir);
    QStringList filters;
    QStringList all_ext;

    QMimeDatabase db;
    for ( const auto& baf : QImageReader::supportedMimeTypes() )
    {
        QMimeType mime(db.mimeTypeForName(baf));
        if ( mime.isValid() )
        {
            const QString patterns = mime.globPatterns().join(QLatin1Char(' '));
            all_ext += patterns;
            filters.push_back(mime.comment() + QLatin1String(" (") + patterns + QLatin1Char(')'));
        }
    }
    filters.push_front(tr("All Supported files (%1)").arg(all_ext.join(' ')));
    dialog.setNameFilters(filters);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::DontUseNativeDialog, !app::settings::get<bool>("open_save", "native_dialog"));

    if ( multiple )
        dialog.setFileMode(QFileDialog::ExistingFiles);
    else
        dialog.setFileMode(QFileDialog::ExistingFile);

    if ( dialog.exec() == QDialog::Rejected )
        return {};

    if ( out_dir )
        *out_dir = dialog.directory().path();

    return dialog.selectedFiles();
}


void GlaxnimateWindow::Private::import_image()
{
    QString path = app::settings::get<QString>("open_save", "import_path");
    if ( path.isEmpty() )
        path = current_document->io_options().path.absolutePath();

    QStringList image_files = get_open_image_files(tr("Import Image"), path, &path, true);
    if ( image_files.isEmpty() )
        return;

    app::settings::set("open_save", "import_path", path);

    /// \todo dialog asking whether to embed
    command::UndoMacroGuard macro(tr("Import Image"), current_document.get());

    model::Image* select = nullptr;

    for ( const auto& image_file : image_files )
    {
        auto bitmap = std::make_unique<model::Bitmap>(current_document.get());
        bitmap->filename.set(image_file);
        if ( bitmap->pixmap().isNull() )
        {
            show_warning(tr("Import Image"), tr("Could not import image"));
            continue;
        }

        auto defs = current_document->assets();
        auto bmp_ptr = bitmap.get();
        current_document->push_command(new command::AddObject(&defs->images->values, std::move(bitmap), defs->images->values.size()));

        auto image = std::make_unique<model::Image>(current_document.get());
        image->image.set(bmp_ptr);
        QPointF p(bmp_ptr->pixmap().width() / 2.0, bmp_ptr->pixmap().height() / 2.0);
        image->transform->anchor_point.set(p);
        image->transform->position.set(p);
        auto comp = current_composition();
        select = image.get();
        image->name.set(QFileInfo(image_file).baseName());
        current_document->push_command(new command::AddShape(&comp->shapes, std::move(image), comp->shapes.size()));
    }

    if ( select )
        set_current_document_node(select);
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

    remove_assets(current_document->assets()->gradients->values, count);
    remove_assets(current_document->assets()->gradient_colors->values, count);
    remove_assets(current_document->assets()->colors->values, count);
    remove_assets(current_document->assets()->images->values, count);

    status_message(tr("Removed %1 assets").arg(count), 0);
}

void GlaxnimateWindow::Private::convert_to_path(const std::vector<model::ShapeElement*>& shapes, std::vector<model::ShapeElement*>* out)
{
    if ( shapes.empty() )
        return;

    QString macro_name = tr("Convert to path");
    if ( shapes.size() == 1 )
        macro_name = tr("Convert %1 to path").arg((*shapes.begin())->name.get());

    std::unordered_map<model::Layer*, model::Layer*> converted_layers;

    command::UndoMacroGuard guard(macro_name, current_document.get(), false);
    for ( auto shape : shapes )
    {
        auto path = shape->to_path();

        if ( out )
            out->push_back(path.get());

        if ( path )
        {
            if ( auto lay = shape->cast<model::Layer>() )
                converted_layers[lay] = static_cast<model::Layer*>(path.get());

            guard.start();
            current_document->push_command(
                new command::AddObject<model::ShapeElement>(
                    shape->owner(),
                    std::move(path),
                    shape->position()
                )
            );
            current_document->push_command(
                new command::RemoveObject<model::ShapeElement>(shape, shape->owner())
            );
        }
    }

    // Maintain parenting of layers that have been converted
    for ( const auto& p : converted_layers )
    {
        if ( auto src_parent = p.first->parent.get() )
        {
            auto it = converted_layers.find(src_parent);
            if ( it != converted_layers.end() )
                p.second->parent.set(it->second);
        }
    }
}

void GlaxnimateWindow::Private::to_path()
{
    std::vector<model::ShapeElement*> shapes;

    for ( auto selected : scene.cleaned_selection() )
    {
        if ( selected->docnode_locked_recursive() )
            continue;

        if ( auto shape = selected->cast<model::ShapeElement>() )
        {
            if ( !shape->cast<model::Styler>() )
                shapes.push_back(shape);
        }
    }

    convert_to_path(shapes, nullptr);
}

void GlaxnimateWindow::Private::switch_composition(model::Composition* new_comp, int i)
{
    if ( i != ui.tab_bar->currentIndex() )
    {
        QSignalBlocker g(ui.tab_bar);
        ui.tab_bar->setCurrentIndex(i);
    }

    if ( comp )
    {
        int old_i = current_document->assets()->compositions->values.index_of(static_cast<model::Composition*>(comp));
        comp_selections[old_i].selection = scene.selection();
        if ( ui.view_document_node->currentIndex().isValid() )
            comp_selections[old_i].current = ui.view_document_node->current_node();
        else
            comp_selections[old_i].current = comp;

        QObject::disconnect(comp->animation.get(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &FrameControlsWidget::set_min);
        QObject::disconnect(comp->animation.get(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &FrameControlsWidget::set_max);;
        QObject::disconnect(comp, &model::Composition::fps_changed, ui.play_controls, &FrameControlsWidget::set_fps);
    }

    comp = new_comp;

    ui.play_controls->set_range(comp->animation->first_frame.get(), comp->animation->last_frame.get());
    ui.play_controls->set_fps(comp->fps.get());
    ui.play_controls_2->set_range(comp->animation->first_frame.get(), comp->animation->last_frame.get());
    ui.play_controls_2->set_fps(comp->fps.get());

    QObject::connect(comp->animation.get(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &FrameControlsWidget::set_min);
    QObject::connect(comp->animation.get(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &FrameControlsWidget::set_max);;
    QObject::connect(comp, &model::Composition::fps_changed, ui.play_controls, &FrameControlsWidget::set_fps);

    auto possible = current_document->comp_graph().possible_descendants(comp, current_document.get());
    std::set<model::Composition*> comps(possible.begin(), possible.end());
    for ( QAction* action : ui.menu_new_comp_layer->actions() )
        action->setEnabled(comps.count(action->data().value<model::Composition*>()));

    ui.view_document_node->set_composition(comp);
    ui.timeline_widget->set_composition(comp);
    scene.set_composition(comp);
    scene.user_select(comp_selections[i].selection, graphics::DocumentScene::Replace);
    auto current = comp_selections[i].current;
    ui.view_document_node->set_current_node(current);

    ui.canvas->viewport()->update();
}

void GlaxnimateWindow::Private::setup_composition(model::Composition* comp, int index)
{
    CompState state;
    if ( !comp->shapes.empty() )
        state = comp->shapes[0];
    else
        state = comp;

    if ( index == -1 )
        index = comp_selections.size();

    comp_selections.insert(comp_selections.begin() + index, std::move(state));

    QAction* action = nullptr;

    ui.menu_new_comp_layer->setEnabled(true);
    action = new QAction(comp->instance_icon(), comp->object_name(), comp);
    if ( ui.menu_new_comp_layer->actions().empty() || index >= ui.menu_new_comp_layer->actions().size() )
        ui.menu_new_comp_layer->addAction(action);
    else
        ui.menu_new_comp_layer->insertAction(ui.menu_new_comp_layer->actions()[index-1], action);
    action->setData(QVariant::fromValue(comp));

    connect(comp, &model::DocumentNode::name_changed, action, [this, comp, action](){
        action->setText(comp->object_name());
    });
    connect(comp, &model::VisualNode::docnode_group_color_changed, action, [this, comp, action](){
            action->setIcon(comp->instance_icon());
    });

}

void GlaxnimateWindow::Private::add_composition()
{
    auto old_comp = this->comp;
    std::unique_ptr<model::Composition> comp = std::make_unique<model::Composition>(current_document.get());

    auto lay = std::make_unique<model::Layer>(current_document.get());
    current_document->set_best_name(lay.get());

    comp->animation->first_frame.set(old_comp->animation->first_frame.get());
    comp->animation->last_frame.set(old_comp->animation->last_frame.get());
    comp->fps.set(old_comp->fps.get());
    comp->width.set(old_comp->width.get());
    comp->height.set(old_comp->height.get());

    QPointF center(comp->width.get() / 2, comp->height.get() / 2);
    lay->transform->anchor_point.set(center);
    lay->transform->position.set(center);
    comp->shapes.insert(std::move(lay));

    current_document->set_best_name(comp.get());
    current_document->push_command(new command::AddObject(&current_document->assets()->compositions->values, std::move(comp)));
    ui.tab_bar->setCurrentIndex(ui.tab_bar->count()-1);
}

void GlaxnimateWindow::Private::objects_to_new_composition(
    model::Composition* comp,
    const std::vector<model::VisualNode*>& objects,
    model::ObjectListProperty<model::ShapeElement>* layer_parent,
    int layer_index
)
{
    if ( objects.empty() )
        return;

    int new_comp_index = current_document->assets()->compositions->values.size();
    command::UndoMacroGuard guard(tr("New Composition from Selection"), current_document.get());

    auto ucomp = std::make_unique<model::Composition>(current_document.get());
    model::Composition* new_comp = ucomp.get();
    new_comp->width.set(comp->width.get());
    new_comp->height.set(comp->height.get());
    new_comp->fps.set(comp->fps.get());
    new_comp->animation->first_frame.set(comp->animation->first_frame.get());
    new_comp->animation->last_frame.set(comp->animation->last_frame.get());
    if ( objects.size() > 1 || objects[0]->name.get().isEmpty() )
        current_document->set_best_name(new_comp);
    else
        new_comp->name.set(objects[0]->name.get());
    current_document->push_command(new command::AddObject(&current_document->assets()->compositions->values, std::move(ucomp)));


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
    pcl->size.set(new_comp->size());
    current_document->set_best_name(pcl.get());
    auto pcl_ptr = pcl.get();
    current_document->push_command(new command::AddShape(layer_parent, std::move(pcl), layer_index));

    switch_composition(new_comp, new_comp_index);

    int old_comp_index = current_document->assets()->compositions->values.index_of(static_cast<model::Composition*>(comp));
    comp_selections[old_comp_index] = pcl_ptr;
}


void GlaxnimateWindow::Private::on_remove_precomp(int index)
{
    model::Composition* precomp = current_document->assets()->compositions->values[index];

    if ( precomp == comp )
    {
        auto& comps = current_document->assets()->compositions->values;
        if ( comps.empty() )
            add_composition();

        switch_composition(comps[qMax(comps.size(), index)], 0);
    }

    delete ui.menu_new_comp_layer->actions()[index];
    comp_selections.erase(comp_selections.begin()+index);
}

void GlaxnimateWindow::Private::layer_new_comp_action(QAction* action)
{
    parent->layer_new_comp(action->data().value<model::Composition*>());
}

void GlaxnimateWindow::Private::shape_to_composition(model::ShapeElement* node)
{
    if ( !node )
        return;

    auto parent = node->docnode_parent();
    if ( !parent )
        return;

    auto ancestor = parent;
    auto grand_ancestor = ancestor->docnode_parent();
    while ( grand_ancestor && !ancestor->is_instance<model::Composition>() )
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

QPointF GlaxnimateWindow::Private::align_point(const QRectF& rect, AlignDirection direction, AlignPosition position)
{
    qreal x;
    qreal y;

    if ( direction == AlignDirection::Horizontal )
    {
        switch ( position )
        {
            case AlignPosition::Begin:  x = rect.left(); break;
            case AlignPosition::Center: x = rect.center().x(); break;
            case AlignPosition::End:    x = rect.right(); break;
        }
        y = rect.center().y();
    }
    else
    {
        switch ( position )
        {
            case AlignPosition::Begin:  y = rect.top(); break;
            case AlignPosition::Center: y = rect.center().y(); break;
            case AlignPosition::End:    y = rect.bottom(); break;
        }
        x = rect.center().x();
    }

    return {x, y};
}

namespace {

struct AlignData
{
    model::VisualNode* node;
    QTransform transform;
    QPointF bounds_point;
};

} // namespace

void GlaxnimateWindow::Private::align(AlignDirection direction, AlignPosition position, bool outside)
{
    std::vector<model::VisualNode*> selection = cleaned_selection();

    if ( selection.empty() )
        return;

    QRectF bounds;

    std::vector<AlignData> data;
    data.reserve(selection.size());

    auto bound_pos = position;
    if ( outside )
    {
        if ( bound_pos == AlignPosition::Begin )
            bound_pos = AlignPosition::End;
        else
            bound_pos = AlignPosition::Begin;
    }

    for ( const auto& item : selection )
    {
        auto t = item->time();
        QRectF local_bounds(item->local_bounding_rect(t));
        if ( !local_bounds.isValid() )
            continue;

        QTransform transform = item->transform_matrix(t);
        auto transformed_bounds = transform.map(local_bounds).boundingRect();
        data.push_back({item, transform.inverted(), align_point(transformed_bounds, direction, bound_pos)});

        if ( !bounds.isValid() )
            bounds = transformed_bounds;
        else
            bounds |= transformed_bounds;
    }

    QPointF reference;

    if ( ui.action_align_to_selection->isChecked() )
    {
        reference = align_point(bounds, direction, position);
    }
    else if ( ui.action_align_to_canvas->isChecked() )
    {
        reference = align_point(comp->rect(), direction, position);
    }
    else if ( ui.action_align_to_canvas_group->isChecked() )
    {
        reference = align_point(comp->rect(), direction, position);
        QPointF bounds_point = align_point(bounds, direction, bound_pos);
        for ( auto& item : data )
            item.bounds_point = bounds_point;
    }

    command::UndoMacroGuard guard(tr("Align Selection"), current_document.get());

    for ( const auto& item : data )
    {
        QPointF target_point = reference;
        if ( direction == AlignDirection::Horizontal )
            target_point.setY(item.bounds_point.y());
        else
            target_point.setX(item.bounds_point.x());

        QPointF delta = item.transform.map(target_point) - item.transform.map(item.bounds_point);

        if ( auto path = item.node->cast<model::Path>() )
        {
            auto bezier = path->shape.get();
            for ( auto& point : bezier )
                point.translate(delta);
            path->shape.set_undoable(QVariant::fromValue(bezier));
        }
        else if ( item.node->has("transform") )
        {
            auto m = item.node->local_transform_matrix(item.node->time());
            auto a = m.map(item.transform.map(target_point));
            auto b = m.map(item.transform.map(item.bounds_point));
            delta = a - b;

            auto trans = item.node->get("transform").value<model::Transform*>();
            auto point = trans->position.get() + delta;
            trans->position.set_undoable(point);
        }
        else if ( item.node->has("position") )
        {
            auto point = item.node->get("position").toPointF() + delta;
            item.node->get_property("position")->set_undoable(point);
        }
    }
}

void GlaxnimateWindow::Private::dropped(const QMimeData* data)
{
    if ( data->hasFormat("application/x.glaxnimate-asset-uuid") )
    {
        command::UndoMacroGuard guard(tr("Drop"), current_document.get(), false);

        item_models::DragDecoder<> decoder(data->data("application/x.glaxnimate-asset-uuid"), current_document.get());

        std::vector<model::VisualNode*> selection;

        auto cont = parent->current_shape_container();
        int position = cont->size();

        for ( auto asset : decoder )
        {
            std::unique_ptr<model::ShapeElement> element;

            if ( auto precomp = asset->cast<model::Composition>() )
            {
                auto layer = std::make_unique<model::PreCompLayer>(current_document.get());
                layer->composition.set(precomp);
                layer->size.set(precomp->size());
                element = std::move(layer);
            }
            else if ( auto bmp = asset->cast<model::Bitmap>() )
            {
                auto image = std::make_unique<model::Image>(current_document.get());
                image->image.set(bmp);
                element = std::move(image);
            }

            if ( element )
            {
                selection.push_back(element.get());
                element->name.set(asset->name.get());
                guard.start();
                current_document->push_command(new command::AddShape(cont, std::move(element), position));
            }
        }

        if ( !selection.empty() )
            scene.user_select(selection, graphics::DocumentScene::Replace);
    }

}

static bool get_text(model::Group* group, std::vector<model::TextShape*>& shapes)
{
    bool found = false;

    for ( const auto& node : group->shapes )
    {
        auto mo = node->metaObject();

        if ( mo->inherits(&model::TextShape::staticMetaObject) )
        {
            shapes.push_back(static_cast<model::TextShape*>(node.get()));
            found = true;
        }
        else if ( mo->inherits(&model::Group::staticMetaObject) )
        {
            found = get_text(static_cast<model::Group*>(node.get()), shapes) && found;
        }
    }

    return found;
}

void GlaxnimateWindow::Private::text_put_on_path()
{
    std::vector<model::TextShape*> shapes;
    model::ShapeElement* path = nullptr;

    for ( const auto& node : scene.selection() )
    {
        auto mo = node->metaObject();

        if ( mo->inherits(&model::TextShape::staticMetaObject) )
        {
            shapes.push_back(static_cast<model::TextShape*>(node));
        }
        else if ( mo->inherits(&model::Group::staticMetaObject) )
        {
            if ( !get_text(static_cast<model::Group*>(node), shapes) )
                path = static_cast<model::ShapeElement*>(node);
        }
        else if (
            mo->inherits(&model::ShapeElement::staticMetaObject) &&
            !mo->inherits(&model::Image::staticMetaObject) &&
            !mo->inherits(&model::PreCompLayer::staticMetaObject) &&
            !mo->inherits(&model::Styler::staticMetaObject)
        )
        {
            path = static_cast<model::ShapeElement*>(node);
        }
    }

    if ( shapes.empty() || !path )
        return;

    command::UndoMacroGuard guard(tr("Put text on path"), current_document.get());
    for ( auto shape : shapes )
        shape->path.set_undoable(QVariant::fromValue(path));
}

void GlaxnimateWindow::Private::text_remove_from_path()
{
    std::vector<model::TextShape*> shapes;

    for ( const auto& node : scene.selection() )
    {
        auto mo = node->metaObject();

        if ( mo->inherits(&model::TextShape::staticMetaObject) )
            shapes.push_back(static_cast<model::TextShape*>(node));
        else if ( mo->inherits(&model::Group::staticMetaObject) )
            get_text(static_cast<model::Group*>(node), shapes);
    }

    if ( shapes.empty() )
        return;

    command::UndoMacroGuard guard(tr("Remove text from path"), current_document.get());
    for ( auto shape : shapes )
        shape->path.set_undoable({});
}
