#include "edit_tool.hpp"

#include <QMenu>
#include <QPointer>

#include "model/shapes/shape.hpp"
#include "graphics/bezier_item.hpp"
#include "graphics/item_data.hpp"
#include "command/animation_commands.hpp"
#include "command/object_list_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "math/bezier/operations.hpp"

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};

class tools::EditTool::Private
{
public:
    enum DragMode
    {
        None,
        Click,
        RubberBand,
        ForwardEvents,
        VertexClick,
        VertexDrag,
        VertexAdd,
    };

    struct Selection
    {
        struct SelectedBezier
        {
            graphics::BezierItem* item;
            QTransform transform;
            QPointF start_point;

            SelectedBezier(graphics::BezierItem* item)
                : item(item)
            {}

            void start_drag(const QPointF& drag_start_point)
            {
                transform = item->target_object()->docnode_fuzzy_parent()->transform_matrix(item->target_object()->time()).inverted();
                start_point = transform.map(drag_start_point);
            }

            void drag(
                const QPointF& scene_pos,
                std::vector<model::AnimatableBase*>& props,
                QVariantList& before,
                QVariantList& after
            )
            {
                props.push_back(item->target_property());

                math::bezier::Bezier bezier = item->bezier();
                before.push_back(QVariant::fromValue(bezier));

                QPointF pos = transform.map(scene_pos);
                QPointF delta = pos - start_point;
                for ( int i : item->selected_indices() )
                    bezier[i].translate(delta);
                after.push_back(QVariant::fromValue(bezier));
                start_point = pos;
            }
        };

        std::map<graphics::BezierItem*, SelectedBezier> selected;
        QPointer<graphics::PointItem> initial = nullptr;

        QPointF drag_start;

        bool empty() const
        {
            return selected.empty();
        }

        void clear()
        {
            initial = nullptr;
            for ( const auto& p : selected )
                p.second.item->clear_selected_indices();
            selected.clear();
        }

        void add_bezier_point_item(graphics::PointItem* item)
        {
            auto grandpa = item->parent_editor();
            if ( selected.find(grandpa) == selected.end() )
                add_bezier_item(grandpa);
            grandpa->select_index(item->index());
        }

        void toggle_handle(graphics::MoveHandle* item)
        {
            auto parent = static_cast<graphics::PointItem*>(item->parentItem());
            auto grandpa = parent->parent_editor();
            auto it = selected.find(grandpa);
            if ( it == selected.end() )
            {
                add_bezier_item(grandpa);
                grandpa->select_index(parent->index());
            }
            else
            {
                grandpa->toggle_index(parent->index());
                if ( grandpa->selected_indices().empty() )
                    selected.erase(it);
            }
        }

        void add_bezier_item(graphics::BezierItem* item)
        {
            selected.emplace(item, SelectedBezier(item));
            QObject::connect(item, &QObject::destroyed, item, [this, item]{
                selected.erase(item);
            });
        }

        void start_drag()
        {
            for ( auto& p : selected )
                p.second.start_drag(drag_start);
        }

        void drag(const MouseEvent& event, bool commit)
        {
            std::vector<model::AnimatableBase*> props;
            QVariantList before;
            QVariantList after;

            for ( auto& p : selected )
            {
                p.second.drag(event.scene_pos, props, before, after);
            }

            event.window->document()->push_command(new command::SetMultipleAnimated(
                QObject::tr("Drag nodes"),
                props,
                before,
                after,
                commit
            ));
        }
    };

    static void impl_extract_selection_recursive_item(graphics::DocumentScene * scene, model::DocumentNode* node)
    {
        auto meta = node->metaObject();
        if ( meta->inherits(&model::Shape::staticMetaObject) || meta->inherits(&model::ShapeOperator::staticMetaObject) )
        {
            scene->show_editors(node);
        }
        else if ( meta->inherits(&model::Group::staticMetaObject) )
        {
            for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
                impl_extract_selection_recursive_item(scene, sub.get());
        }
    }

    static void node_type_action(QMenu* menu, QActionGroup* group, graphics::PointItem* item, math::bezier::PointType type)
    {
        QIcon icon;
        QString label;
        switch ( type )
        {
            case math::bezier::PointType::Corner:
                icon = QIcon::fromTheme("node-type-cusp");
                label = QObject::tr("Cusp");
                break;
            case math::bezier::PointType::Smooth:
                icon = QIcon::fromTheme("node-type-smooth");
                label = QObject::tr("Smooth");
                break;
            case math::bezier::PointType::Symmetrical:
                icon = QIcon::fromTheme("node-type-auto-smooth");
                label = QObject::tr("Symmetrical");
                break;
        }
        QAction* action = menu->addAction(icon, label, item, [item, type, label]{
            auto point = item->point();
            point.type = type;
            point.adjust_handles_from_type();
            item->modify(point, QObject::tr("Set %1 Node").arg(label));
        });

        action->setCheckable(true);
        action->setActionGroup(group);

        if ( item->point().type == type )
            action->setChecked(true);
    }

    static void context_menu(EditTool*thus, const MouseEvent& event)
    {
        auto handle = thus->under_mouse(event, true, SelectionMode::Shape).handle;
        if ( !handle )
            return;

        auto role = handle->role();
        if ( role != graphics::MoveHandle::Vertex && role != graphics::MoveHandle::Tangent )
            return;

        auto item = static_cast<graphics::PointItem*>(handle->parentItem());

        QMenu menu;
        QActionGroup grp(&menu);

        menu.addSection(QObject::tr("Node"));

        node_type_action(&menu, &grp, item, math::bezier::PointType::Corner);
        node_type_action(&menu, &grp, item, math::bezier::PointType::Smooth);
        node_type_action(&menu, &grp, item, math::bezier::PointType::Symmetrical);

        menu.addSeparator();

        if ( role == graphics::MoveHandle::Vertex )
        {
            menu.addAction(QIcon::fromTheme("format-remove-node"), QObject::tr("Remove Node"), item, [item]{
                item->parent_editor()->remove_point(item->index());
            });

            menu.addAction(QIcon::fromTheme("show-node-handles"), QObject::tr("Show Tangents"), item, [item]{
                item->show_tan_in(true);
                item->show_tan_out(true);
            });
        }
        else
        {
            menu.addAction(QIcon::fromTheme("show-node-handles"), QObject::tr("Remove Tangent"), item, [item, handle]{
                item->remove_tangent(handle);
            });
        }

        menu.exec(QCursor::pos());
    }


    DragMode drag_mode;
    QPointF rubber_p1;
    QPointF rubber_p2;
    model::Shape* highlight = nullptr;
    Selection selection;

    math::bezier::ProjectResult insert_params;
    math::bezier::Point insert_preview{{}};
    graphics::BezierItem* insert_item = nullptr;
};

tools::EditTool::EditTool()
    : d(std::make_unique<Private>())
{}

tools::EditTool::~EditTool() = default;

void tools::EditTool::mouse_press(const MouseEvent& event)
{
    d->highlight = nullptr;

    if ( event.press_button == Qt::LeftButton && d->drag_mode == Private::None )
    {
        d->drag_mode = Private::Click;
        d->rubber_p1 = event.event->localPos();

        auto clicked_on = under_mouse(event, true, SelectionMode::Shape);
        if ( clicked_on.handle )
        {
            if ( clicked_on.handle->role() == graphics::MoveHandle::Vertex )
            {
                d->drag_mode = Private::VertexClick;
                d->selection.drag_start = event.scene_pos;
                d->selection.initial = static_cast<graphics::PointItem*>(clicked_on.handle->parentItem());
            }
            else
            {
                d->drag_mode = Private::ForwardEvents;
                event.forward_to_scene();
            }
        }
    }
}

void tools::EditTool::mouse_move(const MouseEvent& event)
{
    if ( event.press_button == Qt::LeftButton )
    {
        switch ( d->drag_mode )
        {
            case Private::None:
                break;
            case Private::ForwardEvents:
                event.forward_to_scene();
                break;
            case Private::Click:
                d->drag_mode = Private::RubberBand;
                [[fallthrough]];
            case Private::RubberBand:
                d->rubber_p2 = event.event->localPos();
                break;
            case Private::VertexClick:
                d->drag_mode = Private::VertexDrag;
                if ( d->selection.initial && !d->selection.initial->parent_editor()->selected_indices().count(d->selection.initial->index()) )
                {
                    graphics::PointItem* initial = d->selection.initial;
                    d->selection.clear();
                    d->selection.add_bezier_point_item(initial);
                }
                d->selection.start_drag();
                [[fallthrough]];
            case Private::VertexDrag:
                d->selection.drag(event, false);
                break;
            case Private::VertexAdd:
                break;
        }
    }
    else if ( event.buttons() == Qt::NoButton )
    {
        if ( d->drag_mode == Private::VertexAdd )
        {
            d->insert_params = {};
            d->insert_item = nullptr;

            if ( d->selection.empty() )
            {
                for ( auto it : event.scene->items() )
                {
                    if ( auto item = qgraphicsitem_cast<graphics::BezierItem*>(it) )
                        d->selection.add_bezier_item(item);
                }
            }

            for ( const auto& it : d->selection.selected )
            {
                auto closest = math::bezier::project(it.first->bezier(), event.scene_pos);
                if ( closest.distance < d->insert_params.distance )
                {
                    d->insert_params = closest;
                    d->insert_item = it.first;
                }
            }

            if ( d->insert_item )
            {
                d->insert_preview = d->insert_item->bezier().split_segment_point(
                    d->insert_params.index, d->insert_params.factor
                );
            }
        }
        else
        {
            d->highlight = nullptr;
            for ( auto node : under_mouse(event, true, SelectionMode::Shape).nodes )
            {
                if ( auto path = node->node()->cast<model::Shape>() )
                {
                    if ( !event.scene->has_editors(path) )
                        d->highlight = path;
                    break;
                }
            }
        }
    }
}

void tools::EditTool::mouse_release(const MouseEvent& event)
{
    if ( event.button() == Qt::LeftButton )
    {
        switch ( d->drag_mode )
        {
            case Private::None:
                break;
            case Private::ForwardEvents:
                event.forward_to_scene();
                break;
            case Private::RubberBand:
            {
                d->rubber_p2 = event.event->localPos();
                d->drag_mode = Private::None;
                if ( !(event.modifiers() & Qt::ShiftModifier) )
                    d->selection.clear();
                auto items = event.scene->items(
                    event.view->mapToScene(
                        QRect(d->rubber_p1.toPoint(), d->rubber_p2.toPoint()).normalized().normalized()
                    ),
                    Qt::IntersectsItemShape,
                    Qt::DescendingOrder,
                    event.view->viewportTransform()
                );
                for ( auto item : items )
                    if ( item->data(graphics::ItemData::HandleRole).toInt() == graphics::MoveHandle::Vertex )
                        d->selection.add_bezier_point_item(static_cast<graphics::PointItem*>(item->parentItem()));
                event.view->viewport()->update();
                break;
            }
            case Private::Click:
            {
                std::vector<model::DocumentNode*> selection;

                auto nodes = under_mouse(event, true, SelectionMode::Group).nodes;

                if ( !nodes.empty() )
                    selection.push_back(nodes[0]->node());

                auto mode = graphics::DocumentScene::Replace;
                if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                    mode = graphics::DocumentScene::Toggle;

                event.scene->user_select(selection, mode);
                break;
            }
            case Private::VertexClick:
                if ( event.modifiers() & Qt::ControlModifier )
                {
                    if ( d->selection.initial->point().type == math::bezier::PointType::Corner )
                        d->selection.initial->set_point_type(math::bezier::PointType::Smooth);
                    else
                        d->selection.initial->set_point_type(math::bezier::PointType::Corner);
                }
                else
                {
                    if ( !(event.modifiers() & Qt::ShiftModifier) )
                        d->selection.clear();
                    d->selection.toggle_handle(handle_under_mouse(event));
                }
                d->selection.initial = nullptr;
                break;
            case Private::VertexDrag:
                d->selection.drag(event, true);
                d->selection.initial = nullptr;
                break;
            case Private::VertexAdd:
                if ( d->insert_item )
                {
                    auto bez = d->insert_item->bezier();
                    bez.split_segment(d->insert_params.index, d->insert_params.factor);
                    event.window->document()->push_command(new command::SetMultipleAnimated(
                        d->insert_item->target_property(),
                        QVariant::fromValue(bez),
                        true
                    ));
                    exit_add_point_mode();
                    emit cursor_changed(Qt::ArrowCursor);
                }
                break;
        }

        d->drag_mode = Private::None;
    }
    else if ( event.button() == Qt::RightButton )
    {
        Private::context_menu(this, event);
    }

}

void tools::EditTool::mouse_double_click(const MouseEvent& event) { Q_UNUSED(event); }

void tools::EditTool::paint(const PaintEvent& event)
{
    if ( d->drag_mode == Private::RubberBand )
    {
        event.painter->setBrush(Qt::transparent);
        QColor select_color = event.view->palette().color(QPalette::Highlight);
        QPen pen(select_color, 1);
        pen.setCosmetic(true);
        event.painter->setPen(pen);
        select_color.setAlpha(128);
        event.painter->setBrush(select_color);
        event.painter->drawRect(QRectF(d->rubber_p1, d->rubber_p2));
    }
    else if ( d->drag_mode == Private::VertexAdd )
    {
        if ( d->insert_item )
        {
            QColor select_color = event.view->palette().color(QPalette::Highlight);
            QPen pen(select_color, 1);
            pen.setCosmetic(true);
            event.painter->setPen(pen);
            event.painter->drawLine(
                event.view->mapFromScene(d->insert_preview.tan_in),
                event.view->mapFromScene(d->insert_preview.tan_out)
            );

            select_color.setAlpha(128);
            event.painter->setBrush(select_color);
            event.painter->drawEllipse(event.view->mapFromScene(d->insert_preview.pos), 6, 6);
        }
    }
    else if ( d->highlight )
    {
        QColor select_color = event.view->palette().color(QPalette::Highlight);
        QPen pen(select_color, 1);
        QPainterPath p;
        d->highlight->to_bezier(d->highlight->time()).add_to_painter_path(p);
        QTransform trans = d->highlight->transform_matrix(d->highlight->time()) * event.view->viewportTransform();
        p = trans.map(p);
        event.painter->setPen(pen);
        event.painter->setBrush(Qt::NoBrush);
        event.painter->drawPath(p);
    }
}

void tools::EditTool::key_press(const KeyEvent& event) { Q_UNUSED(event); }

void tools::EditTool::key_release(const KeyEvent& event)
{
    if ( event.key() == Qt::Key_Escape )
    {
        if ( d->drag_mode == Private::VertexAdd )
        {
            exit_add_point_mode();
            emit cursor_changed(Qt::ArrowCursor);
        }
        event.accept();
    }
}

QCursor tools::EditTool::cursor()
{
    if ( d->drag_mode == Private::VertexAdd )
        return Qt::DragCopyCursor;
    return Qt::ArrowCursor;
}

void tools::EditTool::on_selected(graphics::DocumentScene * scene, model::DocumentNode * node)
{
    Private::impl_extract_selection_recursive_item(scene, node);
}

void tools::EditTool::enable_event(const Event&)
{
    d->highlight = nullptr;
    exit_add_point_mode();
}

void tools::EditTool::disable_event(const Event&)
{
    d->highlight = nullptr;
    exit_add_point_mode();
}

QWidget* tools::EditTool::on_create_widget()
{
    return new QWidget();
}

void tools::EditTool::selection_set_vertex_type(math::bezier::PointType t)
{
    if ( d->selection.empty() )
        return;

    auto doc = d->selection.selected.begin()->first->target_object()->document();
    command::UndoMacroGuard macro(QObject::tr("Set node type"), doc);
    for ( const auto& p : d->selection.selected )
    {
        auto bez = p.first->bezier();
        for ( int index : p.first->selected_indices() )
            bez[index].set_point_type(t);
        p.first->target_property()->set_undoable(QVariant::fromValue(bez));
    }
}

void tools::EditTool::selection_delete()
{
    if ( d->selection.empty() )
        return;

    auto doc = d->selection.selected.begin()->first->target_object()->document();
    command::UndoMacroGuard macro(QObject::tr("Set node type"), doc);

    auto selected = std::move(d->selection.selected);
    d->selection.selected = {};
    for ( const auto& p : selected )
    {
        const auto& bez = p.first->bezier();
        math::bezier::Bezier new_bez;
        new_bez.set_closed(bez.closed());

        for ( int i = 0; i < bez.size(); i++ )
            if ( !p.first->selected_indices().count(i) )
                new_bez.push_back(bez[i]);

        p.first->clear_selected_indices();

        if ( new_bez.size() < 2 && !p.first->target_property()->animated() )
        {
            // At the moment it always is a Path, but it might change in the future (ie: masks)
            if ( auto path = p.first->target_object()->cast<model::Path>() )
            {
                doc->push_command(new command::RemoveObject<model::ShapeElement>(path, path->owner()));
                continue;
            }
        }

        p.first->target_property()->set_undoable(QVariant::fromValue(new_bez));
    }

    d->selection.initial = nullptr;
}

void tools::EditTool::selection_straighten()
{
    if ( d->selection.empty() )
        return;

    auto doc = d->selection.selected.begin()->first->target_object()->document();

    command::UndoMacroGuard macro(QObject::tr("Straighten segments"), doc, false);

    for ( const auto& p : d->selection.selected )
    {
        auto bez = p.first->bezier();
        bool modified = false;
        for ( int index : p.first->selected_indices() )
        {
            int prev_index = index-1;
            if ( index == 0 && bez.closed() )
                prev_index = bez.size() - 1;
            int next_index = index+1;
            if ( index == bez.size()-1 && bez.closed() )
                next_index = 0;

            if ( p.first->selected_indices().count(prev_index) )
            {
                bez[index].tan_in = bez[index].pos;
                bez[index].type = math::bezier::Corner;
                modified = true;
            }

            if ( p.first->selected_indices().count(next_index) )
            {
                bez[index].tan_out = bez[index].pos;
                bez[index].type = math::bezier::Corner;
                modified = true;
            }

        }

        if ( modified )
        {
            if ( !macro.started() )
                macro.start();
            p.first->target_property()->set_undoable(QVariant::fromValue(bez));
        }
    }
}

void tools::EditTool::selection_curve()
{
    if ( d->selection.empty() )
        return;

    auto doc = d->selection.selected.begin()->first->target_object()->document();

    command::UndoMacroGuard macro(QObject::tr("Curve segments"), doc, false);

    for ( const auto& p : d->selection.selected )
    {
        auto bez = p.first->bezier();
        bool modified = false;
        for ( int index : p.first->selected_indices() )
        {
            bool mod_in = false;
            int prev_index = index-1;
            if ( index == 0 && bez.closed() )
                prev_index = bez.size() - 1;

            int next_index = index+1;
            if ( index == bez.size()-1 && bez.closed() )
                next_index = 0;

            if ( bez[index].tan_in == bez[index].pos && p.first->selected_indices().count(prev_index) )
            {
                bez[index].tan_in = math::lerp(bez[index].pos, bez[prev_index].pos, 1/3.);
                modified = mod_in = true;
            }

            if ( bez[index].tan_out == bez[index].pos && p.first->selected_indices().count(next_index) )
            {
                bez[index].tan_out = math::lerp(bez[index].pos, bez[next_index].pos, 1/3.);
                modified = true;
                if ( mod_in )
                {
                    bez[index].set_point_type(math::bezier::Smooth);
                }
            }
        }

        if ( modified )
        {
            if ( !macro.started() )
                macro.start();
            p.first->target_property()->set_undoable(QVariant::fromValue(bez));
        }
    }
}

void tools::EditTool::add_point_mode()
{
    d->drag_mode = Private::VertexAdd;
    emit cursor_changed(cursor());
}

void tools::EditTool::exit_add_point_mode()
{
    d->insert_item = nullptr;
    d->insert_params = {};
    d->drag_mode = Private::None;
}
