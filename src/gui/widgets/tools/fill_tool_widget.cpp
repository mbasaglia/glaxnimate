#include "fill_tool_widget.hpp"
#include "ui_fill_tool_widget.h"
#include <QEvent>

class FillToolWidget::Private
{
public:
    Ui::FillToolWidget ui;
};

FillToolWidget::FillToolWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

FillToolWidget::~FillToolWidget() = default;

void FillToolWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

bool FillToolWidget::fill() const
{
    return !d->ui.check_stroke->isChecked();
}

bool FillToolWidget::stroke() const
{
    return !d->ui.check_fill->isChecked();
}

void FillToolWidget::swap_fill_color()
{
    if ( d->ui.check_stroke->isChecked() )
        d->ui.check_fill->setChecked(true);
    else if ( d->ui.check_fill->isChecked() )
        d->ui.check_stroke->setChecked(true);
}



