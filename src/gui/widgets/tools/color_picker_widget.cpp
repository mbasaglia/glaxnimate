#include "color_picker_widget.hpp"
#include "ui_color_picker_widget.h"

#include <QEvent>

using namespace glaxnimate::gui;
using namespace glaxnimate;

class ColorPickerWidget::Private
{
public:
    Ui::ColorPickerWidget ui;
};

ColorPickerWidget::ColorPickerWidget ( QWidget* parent )
    : QWidget ( parent ), d ( std::make_unique<Private>() )
{
    d->ui.setupUi ( this );
}

ColorPickerWidget::~ColorPickerWidget() = default;

void ColorPickerWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent ( e );

    if ( e->type() == QEvent::LanguageChange ) {
        d->ui.retranslateUi ( this );
    }
}

void ColorPickerWidget::set_color ( const QColor& color )
{
    d->ui.color_preview->setColor(color);
}


bool ColorPickerWidget::set_fill() const
{
    return d->ui.check_fill->isChecked();
}

bool ColorPickerWidget::set_stroke() const
{
    return d->ui.check_stroke->isChecked();
}

void ColorPickerWidget::swap_fill_color()
{
    if ( d->ui.check_fill->isChecked() )
        d->ui.check_stroke->setChecked(true);
    else
        d->ui.check_fill->setChecked(true);
}
