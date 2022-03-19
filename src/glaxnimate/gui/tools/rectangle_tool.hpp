#pragma once
#include "base.hpp"

#include "draw_tool_drag.hpp"
#include "glaxnimate/core/model/shapes/rect.hpp"

namespace glaxnimate::gui::tools {

class RectangleTool : public DrawToolDrag
{
public:
    QString id() const override { return "draw-rect"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-rectangle"); }
    QString name() const override { return QObject::tr("Rectangle"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F4"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Shape; }
    int group() const noexcept override { return static_group(); }

protected:
    void on_drag_start() override
    {
        rect = QRectF(p1, p2);
    }

    void on_drag(const MouseEvent& event) override
    {
        update_rect(event.modifiers());
    }

    void on_drag_complete(const MouseEvent& event) override
    {
        auto shape = std::make_unique<model::Rect>(event.window->document());
        rect = rect.normalized();
        shape->position.set(rect.center());
        shape->size.set(rect.size());
        create_shape(QObject::tr("Draw Rectangle"), event, std::move(shape));
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape || event.key() == Qt::Key_Back )
        {
            dragging = false;
            event.repaint();
        }
        else if ( dragging )
        {
            update_rect(event.modifiers());
            event.repaint();
        }
    }

    void key_release(const KeyEvent& event) override
    {
        if ( dragging )
        {
            update_rect(event.modifiers());
            event.repaint();
        }
    }

    void paint(const PaintEvent& event) override
    {
        if ( dragging )
        {
            QPainterPath path;
            path.addPolygon(event.view->mapFromScene(rect));
            path.closeSubpath();

            draw_shape(event, path);
        }
    }

protected:
    void update_rect(Qt::KeyboardModifiers modifiers)
    {
        QPointF recp2 = p2;

        if ( modifiers & Qt::ControlModifier )
        {
            QPointF dp = recp2 - p1;
            qreal comp = qMax(dp.x(), dp.y());
            recp2 = p1 + QPointF(comp, comp);
        }

        if ( modifiers & Qt::ShiftModifier )
            rect = QRectF(2*p1-recp2, recp2);
        else
            rect = QRectF(p1, recp2);
    }

    QRectF rect;

    static Autoreg<RectangleTool> autoreg;
};

} // namespace glaxnimate::gui::tools
