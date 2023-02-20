#pragma once

#include <QMenu>
#include <QGraphicsItem>

#include "graphics/item_data.hpp"
#include "model/animation/animatable.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "app/application.hpp"
#include "widgets/menus/animated_property_menu.hpp"


namespace glaxnimate::gui::tools {

inline void add_property_menu_actions(QMenu* menu, QGraphicsItem* item, SelectionManager* window)
{
    bool started = false;
    for ( const auto& propvariant : item->data(graphics::AssociatedProperty).toList() )
    {
        auto prop = propvariant.value<model::AnimatableBase*>();
        if ( !prop )
            continue;

        if ( !started )
        {
            menu->addSection(QMenu::tr("Properties"));
            started = true;
        }

        AnimatedPropertyMenu* sub = new AnimatedPropertyMenu(menu);
        sub->set_property(prop);
        sub->set_controller(window);
        menu->addAction(sub->menuAction());

    }
}

} // namespace glaxnimate::gui::tools
