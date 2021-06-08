#pragma once

#include <QWidget>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>

#include "model/assets/brush_style.hpp"
#include "model/assets/gradient.hpp"

class ShapeStylePreviewWidget : public QWidget
{
public:
    ShapeStylePreviewWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void set_fill_color(const QColor& color)
    {
        fill_color = color;
        update();
    }
    void set_stroke_color(const QColor& color)
    {
        stroke_color = color;
        update();
    }
    void set_fill_ref(model::BrushStyle* ref)
    {
        fill_ref = ref;
        update();
    }
    void set_stroke_ref(model::BrushStyle* ref)
    {
        stroke_ref = ref;
        update();
    }

    void clear_gradients()
    {
        clear_gradient(fill_ref);
        clear_gradient(stroke_ref);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        int border = width() / 5.;
        QRectF area(rect());
        draw_rect(painter, area, stroke_ref, stroke_color);
        area.adjust(border, border, -border, -border);
        draw_rect(painter, area, fill_ref, fill_color);
    }

private:
    void clear_gradient(QPointer<model::BrushStyle>& item)
    {
        if ( item && item->is_instance<model::Gradient>() )
            item = {};
    }

    void draw_rect(QPainter& painter, const QRectF& rect, const QPointer<model::BrushStyle>& linked, const QColor& plain)
    {
        static QBrush background(QPixmap(QStringLiteral(":/color_widgets/alphaback.png")));
        painter.fillRect(rect, background);

        QBrush brush = linked ? linked->constrained_brush_style(0, rect) : plain;
        painter.fillRect(rect, brush);
    }

    QColor fill_color;
    QPointer<model::BrushStyle> fill_ref;
    QColor stroke_color;
    QPointer<model::BrushStyle> stroke_ref;
    QSize hecking_size;
};
