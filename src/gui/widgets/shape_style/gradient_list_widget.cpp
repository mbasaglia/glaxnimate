#include "gradient_list_widget.hpp"
#include "ui_gradient_list_widget.h"
#include <QEvent>

class GradientListWidget::Private
{
public:
    Ui::GradientListWidget ui;
};

GradientListWidget::GradientListWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

GradientListWidget::~GradientListWidget() = default;

void GradientListWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}
