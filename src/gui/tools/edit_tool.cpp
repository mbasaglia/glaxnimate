#include "base.hpp"

#include "model/shapes/path.hpp"

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

private:
    DragMode drag_mode;
    QPointF rubber_p1;
    QPointF rubber_p2;
    model::Path* highlight = nullptr;

    static Autoreg<EditTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};
