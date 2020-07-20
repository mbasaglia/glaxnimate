#include "view_transform_widget.hpp"
#include "ui_view_transform_widget.h"
#include <QEvent>
#include <QtMath>

ViewTransformWidget::ViewTransformWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Ui::ViewTransformWidget>())
{
    d->setupUi(this);
    connect(d->btn_angle_reset, &QAbstractButton::clicked, [this](){
        d->spin_angle->setValue(0);
    });
    connect(d->btn_zoom_reset, &QAbstractButton::clicked, [this](){
        d->spin_zoom->setValue(100);
    });
    connect(d->btn_zoom_in, &QAbstractButton::clicked, this, &ViewTransformWidget::zoom_in);
    connect(d->btn_zoom_out, &QAbstractButton::clicked, this, &ViewTransformWidget::zoom_out);
    using func = void (QDoubleSpinBox::*)(qreal);
    connect(d->spin_angle, (func)&QDoubleSpinBox::valueChanged, this, &ViewTransformWidget::fuckyoumoc_on_angle_changed);
    connect(d->spin_zoom, (func)&QDoubleSpinBox::valueChanged, this, &ViewTransformWidget::fuckyoumoc_on_zoom_changed);
    connect(d->btn_view_fit, &QAbstractButton::clicked, this, &ViewTransformWidget::view_fit);

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    d->spin_zoom->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
#endif

}

ViewTransformWidget::~ViewTransformWidget() = default;

void ViewTransformWidget::changeEvent(QEvent* e)
{

    QWidget::changeEvent(e);
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            d->retranslateUi(this);
            break;
        default:
            break;
    }
}

void ViewTransformWidget::set_angle(qreal radians)
{
    bool block = d->spin_angle->blockSignals(true);
    d->spin_angle->setValue(qRadiansToDegrees(radians));
    d->spin_angle->blockSignals(block);
}

void ViewTransformWidget::set_zoom(qreal percent)
{
    bool block = d->spin_zoom->blockSignals(true);
    d->spin_zoom->setValue(percent*100);
    d->spin_zoom->blockSignals(block);
}

void ViewTransformWidget::fuckyoumoc_on_angle_changed(qreal degrees)
{
    emit angle_changed(qDegreesToRadians(degrees));
}

void ViewTransformWidget::fuckyoumoc_on_zoom_changed(qreal percent)
{
    emit zoom_changed(percent/100);
}
