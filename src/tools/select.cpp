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
        None,
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
                return;
            }
        }
        
        rubber_p1 = event.event->localPos();
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
        else if ( drag_mode == Click || drag_mode == RubberBand )
        {
            rubber_p2 = event.event->localPos();
            if ( drag_mode == Click && (rubber_p1 - rubber_p2).manhattanLength() > 4 )
                drag_mode = RubberBand;
        }
    }
    
    void complex_select(const MouseEvent& event, const std::vector<model::graphics::DocumentNodeGraphicsItem*>& items)
    {
        
        auto mode = model::graphics::DocumentScene::Replace;
        if ( event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier) )
            mode = model::graphics::DocumentScene::Append;
        
        std::vector<model::DocumentNode*> selection;
        
        for ( auto item : items )
        {
            if ( item->node()->docnode_selectable() )
                selection.push_back(item->node());
        }
        
        event.scene->user_select(selection, mode);
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
            
            complex_select(event, event.scene->nodes(draw_path, event.view->viewportTransform()));
            draw_path = {};
            event.view->update();
        }
        else if ( drag_mode == RubberBand )
        {
            rubber_p2 = event.event->localPos();
            auto poly = event.view->mapToScene(QRect(rubber_p1.toPoint(), rubber_p2.toPoint()).normalized());
            complex_select(event, event.scene->nodes(poly, event.view->viewportTransform()));
            
            drag_mode = None;
            event.view->update();
        }
        else if ( drag_mode == Click )
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
        
        drag_mode = None;
    }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    
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

    DragMode drag_mode;
    QPainterPath draw_path;
    QPointF rubber_p1;
    QPointF rubber_p2;
    
    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
