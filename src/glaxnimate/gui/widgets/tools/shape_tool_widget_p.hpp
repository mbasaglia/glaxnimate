#pragma once
#include "shape_tool_widget.hpp"
#include "ui_shape_tool_widget.h"

#include "app/settings/widget.hpp"

class glaxnimate::gui::ShapeToolWidget::Private
{
public:

    virtual ~Private() = default;

    void load_settings()
    {
        settings.define();
        check_checks();
        on_load_settings();
    }

    void save_settings()
    {
        settings.save();
        on_save_settings();
    }

    void check_checks()
    {
        if ( !ui.check_group->isChecked() && !ui.check_layer->isChecked() )
        {
            if ( ui.check_fill->isEnabled() )
            {
                old_check_fill = ui.check_fill->isChecked();
                ui.check_fill->setEnabled(false);
                ui.check_fill->setChecked(false);

                old_check_stroke = ui.check_stroke->isChecked();
                ui.check_stroke->setEnabled(false);
                ui.check_stroke->setChecked(false);
            }
        }
        else if ( !ui.check_fill->isEnabled() )
        {
            ui.check_fill->setEnabled(true);
            ui.check_fill->setChecked(old_check_fill);

            ui.check_stroke->setEnabled(true);
            ui.check_stroke->setChecked(old_check_stroke);
        }
    }

    void setup_ui(ShapeToolWidget* parent)
    {
        ui.setupUi(parent);
        settings.add(ui.check_group, "tools", "shape_");
        settings.add(ui.check_layer, "tools", "shape_");
        settings.add(ui.check_raw_shape, "tools", "shape_");
        settings.add(ui.check_fill, "tools", "shape_");
        settings.add(ui.check_stroke, "tools", "shape_");
        on_setup_ui(parent, ui.layout);
    }

    void retranslate(ShapeToolWidget* parent)
    {
        ui.retranslateUi(parent);
        on_retranslate();
    }

    bool create_fill() const
    {
        return ui.check_fill->isChecked();
    }

    bool create_layer() const
    {
        return ui.check_layer->isChecked();
    }

    bool create_group() const
    {
        return ui.check_group->isChecked();
    }

    bool create_stroke() const
    {
        return ui.check_stroke->isChecked();
    }

protected:
    virtual void on_load_settings() {}
    virtual void on_save_settings() {}
    virtual void on_setup_ui(ShapeToolWidget*, QVBoxLayout*) {}
    virtual void on_retranslate() {}

private:
    Ui::ShapeToolWidget ui;
    bool old_check_fill;
    bool old_check_stroke;
    app::settings::WidgetSettingGroup settings;
};
