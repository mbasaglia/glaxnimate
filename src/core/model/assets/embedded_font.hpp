/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "asset.hpp"
#include "model/custom_font.hpp"

namespace glaxnimate::model {


class EmbeddedFont : public Asset
{
    GLAXNIMATE_OBJECT(EmbeddedFont)

    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &EmbeddedFont::on_data_changed)
    GLAXNIMATE_PROPERTY(QString, source_url, {})
    GLAXNIMATE_PROPERTY(QString, css_url, {})

    Q_PROPERTY(QString family READ family)
    Q_PROPERTY(QString style_name READ style_name)
    Q_PROPERTY(int database_index READ database_index)

public:
    EmbeddedFont(model::Document* document);
    EmbeddedFont(model::Document* document, CustomFont custom_font);

    QIcon instance_icon() const override;
    QString type_name_human() const override;
    QString object_name() const override;
    bool remove_if_unused(bool clean_lists) override;


    QString family() const { return custom_font_.family(); }
    QString style_name() const { return custom_font_.style_name(); }
    int database_index() const { return custom_font_.database_index(); }
    const CustomFont& custom_font() const { return custom_font_; }

private:
    void on_data_changed();

    CustomFont custom_font_;
};

} // namespace glaxnimate::model
