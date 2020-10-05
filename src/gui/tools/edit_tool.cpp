#include "base.hpp"

#include <QMenu>
#include <QPointer>

#include "model/shapes/shape.hpp"
#include "graphics/bezier_item.hpp"
#include "graphics/item_data.hpp"

namespace tools {

class EditTool : public Tool
{
public:
    QString id() const override { return "edit"; }
    QIcon icon() const override { return QIcon::fromTheme("edit-node"); }
    QString name() const override { return QObject::tr("Edit"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F2"), QKeySequence::PortableText); }

private:
    enum DragMode
    {
        None,
        Click,
        RubberBand,
        ForwardEvents,
        VertexClick,
        VertexDrag,
    };

    struct Selection
    {
        std::set<graphics::BezierItem*> selected;
        QPointer<graphics::BezierPointItem> initial = nullptr;

        QPointF drag_start;
        QPointF drag_last;

        bool empty() const
        {
            return selected.empty();
        }

        void clear()
        {
            initial = nullptr;
            for ( auto item : selected )
                item->clear_selected_indices();
            selected.clear();
        }

        void add_handle(QGraphicsItem* item)
        {
            auto role = graphics::MoveHandle::HandleRole(item->data(graphics::ItemData::HandleRole).toInt());
            if ( role != graphics::MoveHandle::Vertex )
                return;

            auto parent = static_cast<graphics::BezierPointItem*>(item->parentItem());
            auto grandpa = parent->parent_editor();
            if ( selected.find(grandpa) == selected.end() )
                add_bezier_item(grandpa);
            grandpa->select_index(parent->index());
        }

        void toggle_handle(graphics::MoveHandle* item)
        {
            auto parent = static_cast<graphics::BezierPointItem*>(item->parentItem());
            auto grandpa = parent->parent_editor();
            if ( selected.find(grandpa) == selected.end() )
            {
                add_bezier_item(grandpa);
                grandpa->select_index(parent->index());
            }
            else
            {
                grandpa->toggle_index(parent->index());
            }
        }

        void add_bezier_item(graphics::BezierItem* item)
        {
            selected.emplace(item);
            QObject::connect(item, &QObject::destroyed, item, [this, item]{
                selected.erase(item);
            });
        }
    };

    void mouse_press(const MouseEvent& event) override
    {
        highlight = nullptr;

        if ( event.press_button == Qt::LeftButton )
        {
            drag_mode = Click;
            rubber_p1 = event.event->localPos();

            auto clicked_on = under_mouse(event, true, SelectionMode::Shape);
            if ( clicked_on.handle )
            {
                if ( clicked_on.handle->role() == graphics::MoveHandle::Vertex )
                {
                    drag_mode = VertexClick;
                    selection.drag_start = selection.drag_last = event.scene_pos;
                    selection.initial = static_cast<graphics::BezierPointItem*>(clicked_on.handle->parentItem());
                }
                else
                {
                    drag_mode = ForwardEvents;
                    event.forward_to_scene();
                }
            }
        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( event.press_button == Qt::LeftButton )
        {
            switch ( drag_mode )
            {
                case None:
                    break;
                case ForwardEvents:
                    event.forward_to_scene();
                    break;
                case Click:
                    drag_mode = RubberBand;
                    [[fallthrough]];
                case RubberBand:
                    rubber_p2 = event.event->localPos();
                    break;
                case VertexClick:
                    drag_mode = VertexDrag;
                    [[fallthrough]];
                case VertexDrag:
                    selection.drag_last = event.scene_pos;
                    break;
            }
        }
        else if ( event.buttons() == Qt::NoButton )
        {
            highlight = nullptr;
            for ( auto node : under_mouse(event, true, SelectionMode::Shape).nodes )
            {
                if ( auto path = node->node()->cast<model::Shape>() )
                {
                    if ( !event.scene->has_editors(path) )
                        highlight = path;
                    break;
                }
            }
        }
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            switch ( drag_mode )
            {
                case None:
                    break;
                case ForwardEvents:
                    event.forward_to_scene();
                    break;
                case RubberBand:
                {
                    rubber_p2 = event.event->localPos();
                    drag_mode = None;
                    if ( !(event.modifiers() & Qt::ShiftModifier) )
                        selection.clear();
                    auto items = event.scene->items(
                        event.view->mapToScene(
                            QRect(rubber_p1.toPoint(), rubber_p2.toPoint()).normalized().normalized()
                        ),
                        Qt::IntersectsItemShape,
                        Qt::DescendingOrder,
                        event.view->viewportTransform()
                    );
                    for ( auto item : items )
                        selection.add_handle(item);
                    event.view->viewport()->update();
                    break;
                }
                case Click:
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
                case VertexClick:
                    if ( event.modifiers() & Qt::ControlModifier )
                    {
                        if ( selection.initial->point().type == math::BezierPointType::Corner )
                            selection.initial->set_point_type(math::BezierPointType::Smooth);
                        else
                            selection.initial->set_point_type(math::BezierPointType::Corner);
                    }
                    else
                    {
                        if ( !(event.modifiers() & Qt::ShiftModifier) )
                            selection.clear();
                        selection.toggle_handle(handle_under_mouse(event));
                    }
                    break;
                case VertexDrag:
                    break;
            }
        }
        else if ( event.button() == Qt::RightButton )
        {
            context_menu(event);
        }

    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void paint(const PaintEvent& event) override
    {
        if ( drag_mode == RubberBand )
        {
            event.painter->setBrush(Qt::transparent);
            QColor select_color = event.view->palette().color(QPalette::Highlight);
            QPen pen(select_color, 1);
            pen.setCosmetic(true);
            event.painter->setPen(pen);
            select_color.setAlpha(128);
            event.painter->setBrush(select_color);
            event.painter->drawRect(QRectF(rubber_p1, rubber_p2));
        }
        else if ( highlight )
        {
            QColor select_color = event.view->palette().color(QPalette::Highlight);
            QPen pen(select_color, 1);
            QPainterPath p;
            highlight->to_bezier(highlight->time()).add_to_painter_path(p);
            QTransform trans = highlight->transform_matrix(highlight->time()) * event.view->viewportTransform();
            p = trans.map(p);
            event.painter->setPen(pen);
            event.painter->setBrush(Qt::NoBrush);
            event.painter->drawPath(p);
        }
    }

    void key_press(const KeyEvent& event) override { Q_UNUSED(event); }
    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }
    QCursor cursor() override { return {}; }

    static void impl_extract_selection_recursive_item(graphics::DocumentScene * scene, model::DocumentNode* node)
    {
        auto meta = node->metaObject();
        if ( meta->inherits(&model::Shape::staticMetaObject) )
        {
            scene->show_editors(node);
        }
        else if ( meta->inherits(&model::Group::staticMetaObject) )
        {
            for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
                impl_extract_selection_recursive_item(scene, sub.get());
        }
    }

    void on_selected(graphics::DocumentScene * scene, model::DocumentNode * node) override
    {
        impl_extract_selection_recursive_item(scene, node);
    }

    void enable_event(const Event&) override
    {
        highlight = nullptr;
    }

    void disable_event(const Event&) override
    {
        highlight = nullptr;
    }

    void node_type_action(QMenu* menu, QActionGroup* group, graphics::BezierPointItem* item, math::BezierPointType type)
    {
        QIcon icon;
        QString label;
        switch ( type )
        {
            case math::BezierPointType::Corner:
                icon = QIcon::fromTheme("node-type-cusp");
                label = QObject::tr("Cusp");
                break;
            case math::BezierPointType::Smooth:
                icon = QIcon::fromTheme("node-type-smooth");
                label = QObject::tr("Smooth");
                break;
            case math::BezierPointType::Symmetrical:
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

    void context_menu(const MouseEvent& event)
    {
        auto handle = under_mouse(event, true, SelectionMode::Shape).handle;
        if ( !handle )
            return;

        auto role = handle->role();
        if ( role != graphics::MoveHandle::Vertex && role != graphics::MoveHandle::Tangent )
            return;

        auto item = static_cast<graphics::BezierPointItem*>(handle->parentItem());

        QMenu menu;
        QActionGroup grp(&menu);

        menu.addSection(QObject::tr("Node"));

        node_type_action(&menu, &grp, item, math::BezierPointType::Corner);
        node_type_action(&menu, &grp, item, math::BezierPointType::Smooth);
        node_type_action(&menu, &grp, item, math::BezierPointType::Symmetrical);

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

    QWidget* on_create_widget() override
    {
        return new QWidget();
    }

private:
    DragMode drag_mode;
    QPointF rubber_p1;
    QPointF rubber_p2;
    model::Shape* highlight = nullptr;
    Selection selection;

    static Autoreg<EditTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};
