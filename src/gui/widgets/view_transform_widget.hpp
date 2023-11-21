/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VIEWTRANSFORMWIDGET_H
#define VIEWTRANSFORMWIDGET_H

#include <QWidget>
#include <memory>

namespace glaxnimate::gui {

namespace Ui
{
class ViewTransformWidget;
}


class ViewTransformWidget : public QWidget
{
    Q_OBJECT

public:
    ViewTransformWidget(QWidget* parent=nullptr);

    ~ViewTransformWidget();

public Q_SLOTS:
    void set_zoom(qreal percent);
    void set_angle(qreal radians);
    void set_flip(bool flipped);

Q_SIGNALS:
    void zoom_in();
    void zoom_out();
    void zoom_changed(qreal percent);
    void angle_changed(qreal radians);
    void view_fit();
    void flip_view();

private Q_SLOTS:
    void fuckyoumoc_on_zoom_changed(qreal percent);
    void fuckyoumoc_on_angle_changed(qreal degrees);

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui::ViewTransformWidget> d;
};


} // namespace glaxnimate::gui

#endif // VIEWTRANSFORMWIDGET_H
