#include "base.hpp"

#include <variant>

#include "model/shapes/path.hpp"
#include "command/structure_commands.hpp"

#include "widgets/node_menu.hpp"

namespace tools {

class SelectTool : public Tool
{
public:
    QString id() const override { return "select"; }
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
        DragObject,
    };


    struct DragObjectData
    {
        template<class Prop>
        struct PropData
        {
            Prop* property;
            typename Prop::value_type start_value;
        };
        using Variant = std::variant<
            PropData<model::AnimatedProperty<QPointF>>,
            PropData<model::AnimatedProperty<math::Bezier>>
        >;

        template<class T>
        DragObjectData(model::DocumentNode* node, T* property, const QPointF& scene_pos)
        : transform(node->docnode_fuzzy_parent()->transform_matrix(node->time()).inverted()),
          data(PropData<T>{property, property->get()}),
          start_point(transform.map(scene_pos))
        {}

        static void push(model::DocumentNode* node, const QPointF& scene_pos, std::vector<DragObjectData>& out)
        {
            if ( auto prop = node->get_property("position") )
                out.push_back(DragObjectData(node, static_cast<model::AnimatedProperty<QPointF>*>(prop), scene_pos));
            if ( auto prop = node->get_property("transform") )
                out.push_back(DragObjectData(
                    node,
                    &static_cast<model::SubObjectProperty<model::Transform>*>(prop)->get()->position,
                    scene_pos)
                );
            else if ( auto shape = qobject_cast<model::Path*>(node) )
                out.push_back(DragObjectData(node, &shape->shape, scene_pos));
        }

        void drag(const QPointF& dragged_to, bool commit) const
        {
            QPointF delta = transform.map(dragged_to) - start_point;


            if ( data.index() == 0 )
            {
                std::get<0>(data).property->set_undoable(std::get<0>(data).start_value + delta, commit);
                return;
            }

            math::Bezier new_bezier = std::get<1>(data).start_value;
            for ( auto& point : new_bezier )
                point.translate(delta);
            std::get<1>(data).property->set_undoable(QVariant::fromValue(new_bezier), commit);
        }

        QTransform transform;
        Variant data;
        QPointF start_point;
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
            rubber_p1 = event.event->localPos();

            auto clicked_on = under_mouse(event, true);
            if ( clicked_on.handle )
            {
                drag_mode = ForwardEvents;
                event.forward_to_scene();
                return;
            }
            else if ( !clicked_on.nodes.empty() )
            {
                drag_data.clear();
                replace_selection = nullptr;

                bool drag_selection = false;

                for ( auto node : clicked_on.nodes )
                {
                    if ( event.scene->is_descendant_of_selection(node) )
                    {
                        drag_selection = true;
                        break;
                    }
                }

                if ( drag_selection )
                {
                    for ( auto node : event.scene->cleaned_selection() )
                        DragObjectData::push(node, event.scene_pos, drag_data);
                }
                else
                {
                    replace_selection = clicked_on.nodes[0];
                    DragObjectData::push(clicked_on.nodes[0], event.scene_pos, drag_data);
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
                case DrawSelect:
                    draw_path.lineTo(event.scene_pos);
                    break;
                case Click:
                    if ( !drag_data.empty() )
                    {
                        if ( replace_selection )
                        {
                            event.scene->user_select({replace_selection}, graphics::DocumentScene::Replace);
                            replace_selection = nullptr;
                        }
                        drag_mode = DragObject;
                    }
                    else
                    {
                        drag_mode = RubberBand;
                    }
                    mouse_move(event);
                    break;
                case RubberBand:
                    rubber_p2 = event.event->localPos();
                    break;
                case DragObject:
                    for ( const auto& dragger : drag_data )
                        dragger.drag(event.scene_pos, false);
                    break;
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
            switch ( drag_mode )
            {
                case None:
                    break;
                case ForwardEvents:
                    event.forward_to_scene();
                    break;
                case DrawSelect:
                    draw_path.lineTo(event.scene_pos);
                    complex_select(event, event.scene->nodes(draw_path, event.view->viewportTransform()));
                    draw_path = {};
                    event.view->viewport()->update();
                    break;
                case RubberBand:
                    rubber_p2 = event.event->localPos();
                    complex_select(event, event.scene->nodes(
                        event.view->mapToScene(QRect(rubber_p1.toPoint(), rubber_p2.toPoint()).normalized()),
                        event.view->viewportTransform())
                    );
                    drag_mode = None;
                    event.view->viewport()->update();
                    break;
                case Click:
                {
                    replace_selection = nullptr;

                    std::vector<model::DocumentNode*> selection;

                    for ( auto node : under_mouse(event, true).nodes )
                    {
                        if ( !node->docnode_selection_container() )
                        {
                            selection.push_back(node);
                            break;
                        }
                    }

                    auto mode = graphics::DocumentScene::Replace;
                    if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                        mode = graphics::DocumentScene::Toggle;

                    event.scene->user_select(selection, mode);
                }
                break;
                case DragObject:
                    for ( const auto& dragger : drag_data )
                        dragger.drag(event.scene_pos, true);
                    drag_data.clear();
                    replace_selection = nullptr;
                    break;
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
    void key_release(const KeyEvent& event) override
    {
        if ( drag_mode == None && (event.key() == Qt::Key_Delete || event.key() == Qt::Key_Backspace) )
        {
            event.window->delete_selected();
            event.accept();
        }
    }

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

        QMenu menu;
        auto undo_stack = &event.window->document()->undo_stack();
        menu.addAction(
            QIcon::fromTheme("edit-undo"),
            GlaxnimateWindow::tr("Undo %1").arg(undo_stack->undoText()),
            undo_stack, &QUndoStack::undo
        )->setEnabled(undo_stack->canUndo());
        menu.addAction(
            QIcon::fromTheme("edit-redo"),
            GlaxnimateWindow::tr("Redo %1").arg(undo_stack->redoText()),
            undo_stack, &QUndoStack::redo
        )->setEnabled(undo_stack->canRedo());

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-cut"), GlaxnimateWindow::tr("Cut"),
                       event.window, &GlaxnimateWindow::cut);
        menu.addAction(QIcon::fromTheme("edit-copy"), GlaxnimateWindow::tr("Copy"),
                       event.window, &GlaxnimateWindow::copy);
        menu.addAction(QIcon::fromTheme("edit-paste"), GlaxnimateWindow::tr("Paste"),
                       event.window, &GlaxnimateWindow::paste);

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("edit-delete-remove"), GlaxnimateWindow::tr("Delete"),
                       event.window, &GlaxnimateWindow::delete_selected);


        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("object-group"), GlaxnimateWindow::tr("Group Shapes"),
                       event.window, &GlaxnimateWindow::group_shapes);

        menu.addAction(QIcon::fromTheme("object-ungroup"), GlaxnimateWindow::tr("Ungroup Shapes"),
                       event.window, &GlaxnimateWindow::ungroup_shapes);

        menu.addSeparator();
        menu.addAction(QIcon::fromTheme("selection-move-to-layer-above"), GlaxnimateWindow::tr("Move to..."),
                       event.window, &GlaxnimateWindow::move_to);


        menu.addSeparator();

        model::DocumentNode* preferred = event.window->current_shape();

        for ( auto item : items )
        {
            auto obj_menu = new NodeMenu(item->node(), event.window, &menu);
            if ( item->node() == preferred )
                preferred = nullptr;
            if ( obj_menu->actions().size() > 1 )
                menu.addAction(obj_menu->menuAction());
            else
                delete obj_menu;
        }

        if ( preferred )
            menu.addAction((new NodeMenu(preferred, event.window, &menu))->menuAction());

        menu.exec(QCursor::pos());
    }

    DragMode drag_mode;
    QPainterPath draw_path;
    QPointF rubber_p1;
    QPointF rubber_p2;
    std::vector<DragObjectData> drag_data;
    model::DocumentNode* replace_selection = nullptr;

    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
