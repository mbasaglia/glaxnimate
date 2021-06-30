#pragma once

#include <QMenu>
#include <QGraphicsItem>

#include "graphics/item_data.hpp"
#include "model/animation/animatable.hpp"
#include "model/document.hpp"
#include "command/animation_commands.hpp"
#include "app/application.hpp"


namespace tools {

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
        sub->setIcon(GlaxnimateApp::theme_icon("label"));
        menu->addAction(sub->menuAction());

        if ( prop->has_keyframe(prop->time()) )
        {
            sub->addAction(
                GlaxnimateApp::theme_icon("list-remove"),
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
                QIcon(app::Application::instance()->data_file("images/keyframe/status/key.svg")),
                QMenu::tr("Add Keyframe"),
                thus,
                [prop]{
                    prop->object()->push_command(
                        new command::SetKeyframe(prop, prop->time(), prop->value(), true)
                    );
                }
            );
        }

    }
}

} // namespace tools
