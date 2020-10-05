#include "base.hpp"

#include <QMenu>

#include "model/shapes/shape.hpp"
#include "graphics/bezier_item.hpp"

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
    };

    struct Selection
    {
        model::DocumentNode* owner;
        model::AnimatedProperty<math::Bezier>* property;
        std::vector<int> indices;
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
                drag_mode = ForwardEvents;
                event.forward_to_scene();
                return;
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
            }
        }
        else if ( event.buttons() == Qt::NoButton )
        {
            highlight = nullptr;
            for ( auto node : under_mouse(event, true, SelectionMode::Shape).nodes )
            {
                if ( auto path = node->node()->cast<model::Shape>() )
                {
                    if ( !event.scene->is_selected(path) )
                    {
                        highlight = path;
                        break;
                    }
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
                    rubber_p2 = event.event->localPos();
                    drag_mode = None;
                    event.view->viewport()->update();
                    break;
                case Click:
                {
                    std::vector<model::DocumentNode*> selection;

                    for ( auto node : under_mouse(event, true, SelectionMode::Shape).nodes )
                    {
                        if ( node->node()->is_instance<model::Shape>() )
                        {
                            selection.push_back(node->node());
                            break;
                        }
                    }

                    auto mode = graphics::DocumentScene::Replace;
                    if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                        mode = graphics::DocumentScene::Toggle;

                    event.scene->user_select(selection, mode);
                }
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
        /*if ( drag_mode == RubberBand )
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
        else*/ if ( highlight )
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

    bool show_editors(model::DocumentNode* node) const override
    {
        return node->is_instance<model::Shape>();
    }

    static void impl_extract_selection_recursive_item(model::DocumentNode* node, std::vector<model::DocumentNode*>& extra)
    {
        auto meta = node->metaObject();
        if ( meta->inherits(&model::Shape::staticMetaObject) )
        {
            extra.push_back(node);
        }
        else if ( meta->inherits(&model::Group::staticMetaObject) )
        {
            for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
                impl_extract_selection_recursive_item(sub.get(), extra);
        }
    }

    std::vector<model::DocumentNode *> extract_selection_recursive(graphics::DocumentScene* scene) const
    {
        std::vector<model::DocumentNode*> extra;
        for ( auto obj : scene->selection() )
            impl_extract_selection_recursive_item(obj, extra);
        return extra;
    }

    void enable_event(const Event& event) override
    {
        highlight = nullptr;

        /// \todo keep selection and editors separate in the scene, and have each tool
        /// toggle editors as it sees fit.
        std::vector<model::DocumentNode*> extra = extract_selection_recursive(event.scene);
        event.scene->user_select(extra, graphics::DocumentScene::Append);
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
    std::vector<Selection> selection;

    static Autoreg<EditTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};
