/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "timeline_slider.hpp"

#include <QMouseEvent>

#include <QPainter>

void glaxnimate::android::TimelineSlider::set_slider_size(int size)
{
    slider_size = size;
    update();
}

void glaxnimate::android::TimelineSlider::mousePressEvent(QMouseEvent *ev)
{
    setSliderDown(true);
    mouseMoveEvent(ev);
}

void glaxnimate::android::TimelineSlider::mouseMoveEvent(QMouseEvent *ev)
{
    qreal x = qBound(0, ev->pos().x(), width());
    qreal min_x = slider_size / 2;
    qreal max_x = width() - slider_size / 2;
    qreal max_v = maximum();
    qreal min_v = minimum();
    if ( max_x > min_x && max_v > min_v )
    {
        qreal f = (x - min_x) / (max_x - min_x);
        qreal v = min_v + f * (max_v - min_v);
        setSliderPosition(qRound(v));
        update();
    }
}

void glaxnimate::android::TimelineSlider::mouseReleaseEvent(QMouseEvent *)
{
    setSliderDown(false);
}

void glaxnimate::android::TimelineSlider::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QColor border("#8f8f8f");
    QColor groove("#dedede");
    QColor slider("#f3f3f3");

    painter.setBrush(groove);
    painter.setPen(QPen(border, 1));
    painter.drawRect(0, 0, width(), height());

    qreal min_x = slider_size / 2;
    qreal max_x = width() - slider_size / 2;
    qreal max_v = maximum();
    qreal min_v = minimum();
    if ( max_x > min_x && max_v > min_v )
    {
        qreal f = (value() - min_v) / (max_v - min_v);
        qreal x = f * (max_x - min_x) + min_x;

        painter.setBrush(slider);
        painter.drawRect(x - slider_size / 2, 0, slider_size, height());
    }
}
