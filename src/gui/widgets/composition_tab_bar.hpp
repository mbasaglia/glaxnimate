#pragma once

#include "clickable_tab_bar.hpp"

#include "model/document.hpp"
#include "model/assets/precomposition.hpp"

namespace glaxnimate::gui {

class CompositionTabBar : public ClickableTabBar
{
    Q_OBJECT

public:
    explicit CompositionTabBar(QWidget *parent = nullptr);

    model::Composition* index_to_comp(int index) const;
    model::Precomposition* index_to_precomp(int index) const;

    void set_document(model::Document* document);

    void set_current_composition(model::Composition* comp);

private slots:
    void fw_switch(int i);
    void on_close(int i);
    void on_menu(int i);
    void setup_composition(model::Composition* comp, int index);
    void on_precomp_removed(int index);

private:
    void update_comp_color(int index, model::Composition* comp);

signals:
    void switch_composition(model::Composition* comp, int index);

private:
    model::Document* document = nullptr;
};


} // namespace glaxnimate::gui
