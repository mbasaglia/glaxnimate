#ifndef VIEWTRANSFORMWIDGET_H
#define VIEWTRANSFORMWIDGET_H

#include <QWidget>
#include <memory>

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

public slots:
    void set_zoom(qreal percent);
    void set_angle(qreal radians);
    void set_flip(bool flipped);

signals:
    void zoom_in();
    void zoom_out();
    void zoom_changed(qreal percent);
    void angle_changed(qreal radians);
    void view_fit();
    void flip_view();

private slots:
    void fuckyoumoc_on_zoom_changed(qreal percent);
    void fuckyoumoc_on_angle_changed(qreal degrees);

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui::ViewTransformWidget> d;
};

#endif // VIEWTRANSFORMWIDGET_H
