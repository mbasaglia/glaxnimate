#include "base.hpp"

#include "widgets/node_menu.hpp"

namespace tools {

class SelectTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("edit-select"); }
    QString name() const override { return QObject::tr("Select"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F1"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    enum DragMode
    {
        None,
        Click,
        RubberBand,
        ForwardEvents,
        DrawSelect,
    };

    void mouse_press(const MouseEvent& event) override
    {
        if ( event.press_button == Qt::LeftButton )
        {
            if ( event.modifiers() & Qt::AltModifier )
            {
                drag_mode = DrawSelect;
                draw_path.moveTo(event.scene_pos);
                return;
            }

            drag_mode = Click;

            if ( mouse_on_handle(event) )
            {
                drag_mode = ForwardEvents;
                event.forward_to_scene();
                return;
            }

            rubber_p1 = event.event->localPos();
        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( event.press_button == Qt::LeftButton )
        {
            if ( drag_mode == ForwardEvents )
            {
                event.forward_to_scene();
            }
            else if ( drag_mode == DrawSelect )
            {
                draw_path.lineTo(event.scene_pos);
            }
            else if ( drag_mode == Click || drag_mode == RubberBand )
            {
                rubber_p2 = event.event->localPos();
                if ( drag_mode == Click && (rubber_p1 - rubber_p2).manhattanLength() > 4 )
                    drag_mode = RubberBand;
            }
        }
    }

    void complex_select(const MouseEvent& event, const std::vector<graphics::DocumentNodeGraphicsItem*>& items)
    {

        auto mode = graphics::DocumentScene::Replace;
        if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
            mode = graphics::DocumentScene::Append;

        std::vector<model::DocumentNode*> selection;

        for ( auto item : items )
        {
            if ( item->node()->docnode_selectable() && !item->node()->docnode_selection_container() )
                selection.push_back(item->node());
        }

        event.scene->user_select(selection, mode);
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            if ( drag_mode == ForwardEvents )
            {
                event.forward_to_scene();
            }
            else if ( drag_mode == DrawSelect )
            {
                draw_path.lineTo(event.scene_pos);

                complex_select(event, event.scene->nodes(draw_path, event.view->viewportTransform()));
                draw_path = {};
                event.view->viewport()->update();
            }
            else if ( drag_mode == RubberBand )
            {
                rubber_p2 = event.event->localPos();
                auto poly = event.view->mapToScene(QRect(rubber_p1.toPoint(), rubber_p2.toPoint()).normalized());
                complex_select(event, event.scene->nodes(poly, event.view->viewportTransform()));

                drag_mode = None;
                event.view->viewport()->update();
            }
            else if ( drag_mode == Click )
            {
                std::vector<model::DocumentNode*> selection;

                for ( auto item : event.scene->nodes(event.scene_pos, event.view->viewportTransform()) )
                {
                    if ( item->node()->docnode_selectable() && !item->node()->docnode_selection_container() )
                    {
                        selection.push_back(item->node());
                        break;
                    }
                }

                auto mode = graphics::DocumentScene::Replace;
                if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                    mode = graphics::DocumentScene::Toggle;

                event.scene->user_select(selection, mode);
            }

            drag_mode = None;
        }
        else if ( event.press_button == Qt::RightButton )
        {
            context_menu(event);
        }
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void key_press(const KeyEvent& event) override { Q_UNUSED(event); }
    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }

    void paint(const PaintEvent& event) override
    {
        if ( drag_mode == DrawSelect )
        {
            event.painter->setTransform(event.view->viewportTransform());
            event.painter->setBrush(Qt::transparent);
            QPen pen(event.view->palette().color(QPalette::Highlight), 2);
            pen.setCosmetic(true);
            event.painter->setPen(pen);
            event.painter->drawPath(draw_path);
        }
        else if ( drag_mode == RubberBand )
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
    }

    bool show_editors(model::DocumentNode*) const override
    {
        return true;
    }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

    QCursor cursor() override { return Qt::ArrowCursor; }

    void context_menu(const MouseEvent& event)
    {
        auto items = event.scene->nodes(event.scene_pos, event.view->viewportTransform());
        model::DocumentNode* preferred = event.window->current_shape();
        model::DocumentNode* best = nullptr;
        for ( auto item : items )
        {
            if ( !best )
            {
                best = item->node();
            }
            else if ( item->node() == preferred )
            {
                best = preferred;
                break;
            }
            else if ( item->isSelected() )
            {
                best = item->node();
            }
        }

        QMenu menu;
        auto undo_stack = &event.window->document()->undo_stack();
        menu.addAction(QIcon::fromTheme("edit-undo"), undo_stack->undoText(),
                       undo_stack, &QUndoStack::undo)->setEnabled(undo_stack->canUndo());
        menu.addAction(QIcon::fromTheme("edit-redo"), undo_stack->redoText(),
                       undo_stack, &QUndoStack::redo)->setEnabled(undo_stack->canRedo());

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-copy"), GlaxnimateWindow::tr("Copy"),
                       event.window, &GlaxnimateWindow::copy);
        menu.addAction(QIcon::fromTheme("edit-paste"), GlaxnimateWindow::tr("Paste"),
                       event.window, &GlaxnimateWindow::paste);

        if ( best )
        {
            menu.addSeparator();
            auto obj_menu = new NodeMenu(best, &menu);
            if ( obj_menu->actions().size() > 1 )
                menu.addAction(obj_menu->menuAction());
            else
                delete obj_menu;
        }

        menu.exec(event.press_screen_pos);
    }

    DragMode drag_mode;
    QPainterPath draw_path;
    QPointF rubber_p1;
    QPointF rubber_p2;
    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
