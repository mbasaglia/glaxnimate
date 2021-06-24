#include "shape_tool_widget_p.hpp"


ShapeToolWidget::ShapeToolWidget(QWidget* parent)
    : ShapeToolWidget(std::make_unique<Private>(), parent)
{}

ShapeToolWidget::~ShapeToolWidget() = default;

void ShapeToolWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange )
    {
        d->retranslate(this);
    }
}

void ShapeToolWidget::check_checks()
{
    d->check_checks();
    d->save_settings();
    emit checks_changed();
}

bool ShapeToolWidget::create_fill() const
{
    return d->create_fill();
}

bool ShapeToolWidget::create_group() const
{
    return d->create_group();
}

bool ShapeToolWidget::create_stroke() const
{
    return d->create_stroke();
}

bool ShapeToolWidget::create_layer() const
{
    return d->create_layer();
}

void ShapeToolWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->load_settings();
}

ShapeToolWidget::ShapeToolWidget(std::unique_ptr<Private> dd, QWidget* parent)
    : QWidget(parent), d(std::move(dd))
{
    d->setup_ui(this);
    d->load_settings();
}

void ShapeToolWidget::save_settings()
{
    d->save_settings();
}
