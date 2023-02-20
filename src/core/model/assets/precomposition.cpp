/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "precomposition.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"

using namespace glaxnimate;

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Precomposition)

QIcon glaxnimate::model::Precomposition::tree_icon() const
{
    return QIcon::fromTheme("component");
}

QString glaxnimate::model::Precomposition::type_name_human() const
{
    return tr("Composition");
}

bool glaxnimate::model::Precomposition::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->precompositions->values
        ));
        return true;
    }
    return false;
}

glaxnimate::model::DocumentNode * glaxnimate::model::Precomposition::docnode_parent() const
{
    return document()->assets()->precompositions.get();
}
