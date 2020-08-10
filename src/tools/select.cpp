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
    void mouse_press(const MouseEvent& event) override
    {
        for ( auto item : event.scene->items(event.scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, event.view->viewportTransform()) )
        {
            if ( item->flags() & QGraphicsItem::ItemIsFocusable && !event.scene->item_to_node(item) )
            {
                forward = true;
                event.forward_to_scene();
                break;
            }
        }
    }
    void mouse_move(const MouseEvent& event) override
    {
        if ( forward )
        {
            event.forward_to_scene();
        }
        else
        {
        }
    }
    
    void mouse_release(const MouseEvent& event) override
    {
        if ( forward )
        {
            event.forward_to_scene();
            forward = false;
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
            
            event.scene->user_select(selection, mode );
        }
    }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    bool forward = false;
    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
