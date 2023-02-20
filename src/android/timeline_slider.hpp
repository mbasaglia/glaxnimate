/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TIMELINE_SLIDER_HPP
#define TIMELINE_SLIDER_HPP

#include <QAbstractSlider>

namespace glaxnimate::android {

class TimelineSlider : public QAbstractSlider
{
    Q_OBJECT

public:
    using QAbstractSlider::QAbstractSlider;

    void set_slider_size(int size);

protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void paintEvent(QPaintEvent* ev) override;

    int slider_size = 80;
};


} // namespace glaxnimate::android

#endif // TIMELINE_SLIDER_HPP
