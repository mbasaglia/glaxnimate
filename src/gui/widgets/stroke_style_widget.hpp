#ifndef STROKESTYLEWIDGET_H
#define STROKESTYLEWIDGET_H

#include <memory>
#include <QWidget>

#include "model/shapes/stroke.hpp"


class StrokeStyleWidget : public QWidget
{
    Q_OBJECT

public:
    StrokeStyleWidget(QWidget* parent = nullptr);

    ~StrokeStyleWidget();

    void save_settings() const;

    void set_shape(model::Stroke* stroke);

    void set_document(model::Document* document);

    QPen pen_style() const;

protected:
    void changeEvent ( QEvent* e ) override;
    void paintEvent(QPaintEvent * event) override;

public slots:
    void set_color(const QColor& color);

signals:
    void color_changed(const QColor& color);

private slots:
    void check_cap();
    void check_join();
    void check_color(const QColor& color);
    void color_committed(const QColor& color);
    void property_changed(const model::BaseProperty* prop);
    void check_width(double w);
    void check_miter(double w);
    void commit_width();
    void set_target_def(model::BrushStyle* def);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // STROKESTYLEWIDGET_H
