#pragma once
#include "color_selector.hpp"
#include "model/shapes/fill.hpp"
#include "utils/pseudo_mutex.hpp"

class FillStyleWidget : public ColorSelector
{
public:
    FillStyleWidget(QWidget* parent = nullptr);

    void set_shape(model::Fill* target, int gradient_stop = 0);
    model::Fill* shape() const;

    void set_gradient_stop(model::Styler* styler, int index);

private:
    void update_from_target();

    void set_color(const QColor& color, bool commit);

private slots:
    void set_target_color(const QColor& color);

    void commit_target_color();

    void property_changed(const model::BaseProperty* prop);

private:
    model::Fill* target = nullptr;
    utils::PseudoMutex updating;
    int stop = 0;
};

