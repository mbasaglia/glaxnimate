#include "base.hpp"

#include <QGraphicsSceneMouseEvent>

#include "graphics/item_data.hpp"
#include "model/shapes/precomp_layer.hpp"

void tools::MouseEvent::forward_to_scene() const
{
    QEvent::Type type;
    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
            type = QEvent::GraphicsSceneMousePress;
            break;
        case QEvent::MouseMove:
            type = QEvent::GraphicsSceneMouseMove;
            break;
        case QEvent::MouseButtonRelease:
            type = QEvent::GraphicsSceneMouseRelease;
            break;
        case QEvent::MouseButtonDblClick:
            type = QEvent::GraphicsSceneMouseDoubleClick;
            break;
        default:
            return;
    }
    
    QGraphicsSceneMouseEvent mouse_event(type);
    mouse_event.setWidget(view->viewport());
    mouse_event.setButtonDownScenePos(press_button, press_scene_pos);
    mouse_event.setButtonDownScreenPos(press_button, press_screen_pos);
    mouse_event.setScenePos(scene_pos);
    mouse_event.setScreenPos(event->screenPos().toPoint());
    mouse_event.setLastScenePos(last_scene_pos);
    mouse_event.setLastScreenPos(last_screen_pos);
    mouse_event.setButtons(event->buttons());
    mouse_event.setButton(event->button());
    mouse_event.setModifiers(event->modifiers());
    mouse_event.setSource(event->source());
    mouse_event.setFlags(event->flags());
    mouse_event.setAccepted(false);
    QCoreApplication::sendEvent(scene, &mouse_event);
    
    event->setAccepted(mouse_event.isAccepted());
}

tools::Tool::UnderMouse tools::Tool::under_mouse(const tools::MouseEvent& event, bool only_selectable, SelectionMode mode) const
{
    UnderMouse ret;
    for ( auto item : event.scene->items(event.scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, event.view->viewportTransform()) )
    {
        if ( !(item->flags() & (QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable)) )
            continue;

        if ( auto node = event.scene->item_to_node(item) )
        {
            if ( !only_selectable || node->docnode_selectable() )
            {
                auto dnitem = static_cast<graphics::DocumentNodeGraphicsItem*>(item);
                if ( !only_selectable || dnitem->selection_mode() >= mode )
                    ret.nodes.push_back(dnitem);
            }
        }
        else if ( !ret.handle && item->data(graphics::ItemData::HandleRole).toInt() )
        {
            ret.handle = static_cast<graphics::MoveHandle*>(item);
        }
    }

    return ret;
}

graphics::MoveHandle * tools::Tool::handle_under_mouse(const tools::MouseEvent& event) const
{

    for ( auto item : event.scene->items(event.scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, event.view->viewportTransform()) )
    {
        if ( !(item->flags() & (QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable)) )
            continue;

        if ( item->data(graphics::ItemData::HandleRole).toInt() )
            return static_cast<graphics::MoveHandle*>(item);
    }

    return nullptr;
}


void tools::Tool::edit_clicked(const tools::MouseEvent& event)
{
    auto mode = event.modifiers() & Qt::ControlModifier ? SelectionMode::Shape : SelectionMode::Group;
    auto clicked_on = under_mouse(event, true, mode).nodes;

    if ( !clicked_on.empty() )
    {
        auto selected = clicked_on[0]->node();
        if ( auto precomp = selected->cast<model::PreCompLayer>() )
        {
            if ( precomp->composition.get() )
                event.window->set_current_composition(precomp->composition.get());
        }
        else
        {
            event.scene->user_select({selected}, graphics::DocumentScene::Replace);
            event.window->switch_tool(Registry::instance().tool("edit"));
        }
    }
}

void tools::Tool::on_deselected(graphics::DocumentScene* scene, model::VisualNode* node)
{
    scene->hide_editors(node, true, true);
}

