#pragma once
#include "color_selector.hpp"
#include "model/shapes/fill.hpp"
#include "utils/pseudo_mutex.hpp"

namespace glaxnimate::gui {

class FillStyleWidget : public ColorSelector
{
public:
    FillStyleWidget(QWidget* parent = nullptr);

    void set_targets(const std::vector<model::Fill*>& targets);
    model::Fill* current() const;
    void set_current(model::Fill* current);

    void set_gradient_stop(model::Styler* styler, int index);

private:
    void update_from_target();

    void set_color(const QColor& color, bool commit);

    void before_set_target();
    void after_set_target();

private slots:
    void set_target_color(const QColor& color);

    void commit_target_color();

    void property_changed(const model::BaseProperty* prop);

    void clear_target_color();

private:
    std::vector<model::Styler*> targets;
    model::Styler* current_target = nullptr;
    utils::PseudoMutex updating;
    int stop = -1;
};

} // namespace glaxnimate::gui
