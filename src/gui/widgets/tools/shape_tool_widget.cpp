#include "shape_tool_widget.hpp"
#include "ui_shape_tool_widget.h"

#include <QEvent>

#include "app/settings/settings.hpp"

class ShapeToolWidget::Private
{
public:
    Ui::ShapeToolWidget ui;

    void load_settings()
    {
        ui.check_group->setChecked(app::settings::get<bool>("tools", "shape_group"));
        ui.check_fill->setChecked(app::settings::get<bool>("tools", "shape_fill"));
        ui.check_stroke->setChecked(app::settings::get<bool>("tools", "shape_stroke"));
        ui.check_transform->setChecked(app::settings::get<bool>("tools", "shape_transform"));
        check_checks();
    }

    void save_settings()
    {
        app::settings::set("tools", "shape_group", ui.check_group->isChecked());
        app::settings::set("tools", "shape_fill", ui.check_fill->isChecked());
        app::settings::set("tools", "shape_stroke", ui.check_stroke->isChecked());
        app::settings::set("tools", "shape_transform", ui.check_transform->isChecked());
    }

    void check_checks()
    {
        if ( !ui.check_group->isChecked() )
        {
            if ( ui.check_fill->isEnabled() )
            {
                old_check_fill = ui.check_fill->isChecked();
                ui.check_fill->setEnabled(false);
                ui.check_fill->setChecked(false);

                old_check_stroke = ui.check_stroke->isChecked();
                ui.check_stroke->setEnabled(false);
                ui.check_stroke->setChecked(false);

                old_check_transform = ui.check_transform->isChecked();
                ui.check_transform->setEnabled(false);
                ui.check_transform->setChecked(false);
            }
        }
        else if ( !ui.check_fill->isEnabled() )
        {
            ui.check_fill->setEnabled(true);
            ui.check_fill->setChecked(old_check_fill);

            ui.check_stroke->setEnabled(true);
            ui.check_stroke->setChecked(old_check_stroke);

            ui.check_transform->setEnabled(true);
            ui.check_transform->setChecked(old_check_transform);
        }
    }

    bool old_check_fill, old_check_stroke, old_check_transform;
};

ShapeToolWidget::ShapeToolWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->load_settings();
}

ShapeToolWidget::~ShapeToolWidget() = default;


void ShapeToolWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange )
    {
        d->ui.retranslateUi(this);
    }
}

void ShapeToolWidget::check_checks()
{
    d->check_checks();
    d->save_settings();
}

bool ShapeToolWidget::create_fill() const
{
    return d->ui.check_fill->isChecked();
}

bool ShapeToolWidget::create_group() const
{
    return d->ui.check_group->isChecked();
}

bool ShapeToolWidget::create_stroke() const
{
    return d->ui.check_stroke->isChecked();
}

void ShapeToolWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->load_settings();
}

bool ShapeToolWidget::create_transform() const
{
    return d->ui.check_transform->isChecked();
}
