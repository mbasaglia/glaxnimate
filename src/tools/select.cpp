#include "base.hpp"

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
        Click,
        RubberBand,
        ForwardEvents,
        DrawSelect,
    };
    
    void mouse_press(const MouseEvent& event) override
    {
        if ( event.modifiers() & Qt::AltModifier )
        {
            drag_mode = DrawSelect;
            draw_path.moveTo(event.scene_pos);
            return;
        }
        
        drag_mode = Click;
        
        for ( auto item : event.scene->items(event.scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, event.view->viewportTransform()) )
        {
            if ( item->flags() & QGraphicsItem::ItemIsFocusable && !event.scene->item_to_node(item) )
            {
                drag_mode = ForwardEvents;
                event.forward_to_scene();
                break;
            }
        }
    }
    void mouse_move(const MouseEvent& event) override
    {
        if ( drag_mode == ForwardEvents )
        {
            event.forward_to_scene();
        }
        else if ( drag_mode == DrawSelect )
        {
            draw_path.lineTo(event.scene_pos);
        }
    }
    
    void mouse_release(const MouseEvent& event) override
    {
        if ( drag_mode == ForwardEvents )
        {
            event.forward_to_scene();
        }
        else if ( drag_mode == DrawSelect )
        {
            draw_path.lineTo(event.scene_pos);
            
            auto mode = model::graphics::DocumentScene::Replace;
            if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                mode = model::graphics::DocumentScene::Append;
         
            std::vector<model::DocumentNode*> selection;
            
            for ( auto item : event.scene->nodes(draw_path, event.view->viewportTransform()) )
            {
                if ( item->node()->docnode_selectable() )
                    selection.push_back(item->node());
            }
            
            event.scene->user_select(selection, mode);
            draw_path = {};
            event.view->update();
        }
        else
        {
            std::vector<model::DocumentNode*> selection;
            
            for ( auto item : event.scene->nodes(event.scene_pos, event.view->viewportTransform()) )
            {
                if ( item->node()->docnode_selectable() )
                {
                    selection.push_back(item->node());
                    break;
                }   
            }
            
            auto mode = model::graphics::DocumentScene::Replace;
            if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
                mode = model::graphics::DocumentScene::Toggle;
            
            event.scene->user_select(selection, mode);
        }
        
        drag_mode = Click;
    }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    
    void paint(const PaintEvent& event) override
    {
        if ( drag_mode == DrawSelect )
        {
            event.painter->setTransform(event.view->viewportTransform());
            event.painter->setBrush(Qt::transparent);
            QPen p(QColor(0xa0, 0x30, 0x30), 1);
            p.setCosmetic(true);
            event.painter->setPen(p);
            event.painter->drawPath(draw_path);
        }
    }

    DragMode drag_mode;
    QPainterPath draw_path;
    
    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
