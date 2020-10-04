#include "base.hpp"

#include <QMenu>

#include "model/shapes/path.hpp"
#include "graphics/bezier_item.hpp"

namespace tools {

class EditTool : public Tool
{
public:
    QString id() const override { return "edit"; }
    QIcon icon() const override { return QIcon::fromTheme("edit-node"); }
    QString name() const override { return QObject::tr("Edit"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F2"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    enum DragMode
    {
        None,
        Click,
        RubberBand,
        ForwardEvents,
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
                if ( auto path = qobject_cast<model::Path*>(node->node()) )
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
                        if ( qobject_cast<model::Path*>(node->node()) )
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
            highlight->shape.get().add_to_painter_path(p);
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
        return qobject_cast<model::Path*>(node);
    }

    void enable_event(const Event& event) override
    {
        Q_UNUSED(event)
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
        if ( handle->role() != graphics::MoveHandle::Vertex && handle->role() != graphics::MoveHandle::Tangent )
            return;

        auto item = static_cast<graphics::BezierPointItem*>(handle->parentItem());

        QMenu menu;
        QActionGroup grp(&menu);

        menu.addSection(QObject::tr("Node"));

        node_type_action(&menu, &grp, item, math::BezierPointType::Corner);
        node_type_action(&menu, &grp, item, math::BezierPointType::Smooth);
        node_type_action(&menu, &grp, item, math::BezierPointType::Symmetrical);

        menu.addSeparator();

        menu.addAction(QIcon::fromTheme("format-remove-node"), QObject::tr("Remove Node"), item, [item]{
            item->parent_editor()->remove_point(item->index());
        });

        menu.exec(QCursor::pos());
    }

private:
    DragMode drag_mode;
    QPointF rubber_p1;
    QPointF rubber_p2;
    model::Path* highlight = nullptr;

    static Autoreg<EditTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};
