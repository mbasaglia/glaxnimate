#pragma once
#include "color_selector.hpp"
#include "model/shapes/fill.hpp"

class FillStyleWidget : public ColorSelector
{
public:
    FillStyleWidget(QWidget* parent = nullptr);


    void set_shape(model::Fill* target);

private:
    void update_from_target();

    void set_color(const QColor& color, bool commit);

private slots:
    void set_target_color(const QColor& color);

    void commit_target_color();

    void property_changed(const model::BaseProperty* prop);

    void set_target_def(model::BrushStyle* def);

private:
    model::Fill* target = nullptr;
    bool updating = false;
};

