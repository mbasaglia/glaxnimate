#pragma once

#include <QMenu>
#include <QGraphicsItem>

#include "graphics/item_data.hpp"
#include "model/animation/animatable.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "app/application.hpp"


namespace glaxnimate::gui::tools {

inline void add_property_menu_actions(QObject* thus, QMenu* menu, QGraphicsItem* item)
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

        QMenu* sub = new QMenu(prop->name(), menu);
        sub->setIcon(QIcon::fromTheme("label"));
        menu->addAction(sub->menuAction());

        if ( prop->has_keyframe(prop->time()) )
        {
            sub->addAction(
                QIcon(app::Application::instance()->data_file("images/keyframe/status/keyframe-remove.svg")),
                QMenu::tr("Remove Keyframe"),
                thus,
                [prop]{
                    prop->object()->push_command(
                        new command::RemoveKeyframeTime(prop, prop->time())
                    );
                }
            );
        }
        else
        {
            sub->addAction(
                QIcon(app::Application::instance()->data_file("images/keyframe/status/keyframe-add.svg")),
                QMenu::tr("Add Keyframe"),
                thus,
                [prop]{
                    prop->object()->push_command(
                        new command::SetKeyframe(prop, prop->time(), prop->value(), true)
                    );
                }
            );
        }


        if ( prop->animated() )
        {
            sub->addAction(
                QIcon(app::Application::instance()->data_file("images/keyframe/status/keyframe-remove.svg")),
                QMenu::tr("Clear Animations"),
                thus,
                [prop]{
                    prop->object()->push_command(
                        new command::RemoveAllKeyframes(prop)
                    );
                }
            );
        }

    }
}

} // namespace glaxnimate::gui::tools
