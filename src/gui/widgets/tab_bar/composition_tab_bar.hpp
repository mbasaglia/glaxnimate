/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "clickable_tab_bar.hpp"

#include "model/document.hpp"
#include "model/assets/composition.hpp"

namespace glaxnimate::gui {

class CompositionTabBar : public ClickableTabBar
{
    Q_OBJECT

public:
    explicit CompositionTabBar(QWidget *parent = nullptr);

    model::Composition* index_to_comp(int index) const;

    void set_document(model::Document* document);

    void set_current_composition(model::Composition* comp);

private Q_SLOTS:
    void fw_switch(int i);
    void on_close(int i);
    void on_menu(int i);
    void setup_composition(model::Composition* comp, int index);
    void on_precomp_removed(int index);

private:
    void update_comp_color(int index, model::Composition* comp);

Q_SIGNALS:
    void switch_composition(model::Composition* comp, int index);

private:
    model::Document* document = nullptr;
};


} // namespace glaxnimate::gui
