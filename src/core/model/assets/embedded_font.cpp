/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "embedded_font.hpp"

#include "model/document.hpp"
#include "command/object_list_commands.hpp"
#include "model/assets/assets.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::EmbeddedFont)


glaxnimate::model::EmbeddedFont::EmbeddedFont(model::Document* document)
    : Asset(document)
{
}

glaxnimate::model::EmbeddedFont::EmbeddedFont(model::Document* document, CustomFont custom_font)
    : Asset(document), custom_font_(std::move(custom_font))
{
    this->data.set(this->custom_font_.data());
    this->source_url.set(this->custom_font_.source_url());
    this->css_url.set(this->custom_font_.css_url());
}


QIcon glaxnimate::model::EmbeddedFont::instance_icon() const
{
    return QIcon::fromTheme("font");
}

QString glaxnimate::model::EmbeddedFont::object_name() const
{
    return custom_font_.family() + " " + custom_font_.style_name();
}

QString glaxnimate::model::EmbeddedFont::type_name_human() const
{
    return tr("Font");
}

bool glaxnimate::model::EmbeddedFont::remove_if_unused(bool clean_lists)
{
    /// \todo Needs a way to keep track users...
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->fonts->values
        ));
        return true;
    }
    return false;
}

void glaxnimate::model::EmbeddedFont::on_data_changed()
{
    custom_font_ = CustomFontDatabase::instance().add_font("", data.get());
}

