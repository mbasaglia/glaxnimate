#ifndef STROKESTYLEWIDGET_H
#define STROKESTYLEWIDGET_H

#include <memory>
#include <QWidget>

#include "model/shapes/stroke.hpp"


namespace color_widgets {
class ColorPaletteModel;
} // namespace color_widgets

namespace glaxnimate::gui {

class StrokeStyleWidget : public QWidget
{
    Q_OBJECT

public:
    StrokeStyleWidget(QWidget* parent = nullptr);

    ~StrokeStyleWidget();

    void save_settings() const;

    void set_shape(model::Stroke* stroke, int gradient_stop = 0);
    model::Stroke* shape() const;

    void set_stroke_width(qreal w);

    void set_gradient_stop(model::Styler* styler, int index);

    QPen pen_style() const;
    QColor current_color() const;

    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

protected:
    void changeEvent ( QEvent* e ) override;
    void paintEvent(QPaintEvent * event) override;

public slots:
    void set_color(const QColor& color);

signals:
    void color_changed(const QColor& color);
    void pen_style_changed();

private slots:
    void check_cap();
    void check_join();
    void check_color(const QColor& color);
    void color_committed(const QColor& color);
    void property_changed(const model::BaseProperty* prop);
    void check_width(double w);
    void check_miter(double w);
    void commit_width();
    void clear_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
#endif // STROKESTYLEWIDGET_H
