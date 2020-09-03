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
        ui.spin_stroke_width->setValue(app::settings::get<double>("tools", "stroke_width"));
        check_checks();
    }

    void save_settings()
    {
        app::settings::set("tools", "shape_group", ui.check_group->isChecked());
        app::settings::set("tools", "shape_fill", ui.check_fill->isChecked());
        app::settings::set("tools", "shape_stroke", ui.check_stroke->isChecked());
        app::settings::set("tools", "stroke_width", ui.spin_stroke_width->value());
    }

    void check_checks()
    {
        if ( !ui.check_group->isChecked() )
        {
            ui.check_fill->setEnabled(false);
            ui.check_fill->setChecked(false);
            ui.check_stroke->setEnabled(false);
            ui.check_stroke->setChecked(false);
            ui.spin_stroke_width->setEnabled(false);
        }
        else
        {
            ui.check_fill->setEnabled(true);
            ui.check_stroke->setEnabled(true);
            ui.spin_stroke_width->setEnabled(ui.check_stroke->isChecked());
        }
    }
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

qreal ShapeToolWidget::stroke_width() const
{
    return d->ui.spin_stroke_width->value();
}

void ShapeToolWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->load_settings();
}







