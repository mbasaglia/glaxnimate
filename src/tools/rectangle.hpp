#pragma once
#include "base.hpp"

#include "draw_tool_base.hpp"
#include "model/shapes/rect.hpp"

namespace tools {

class RectangleTool : public DrawToolBase
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-rectangle"); }
    QString name() const override { return QObject::tr("Rectangle"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F4"), QKeySequence::PortableText); }

protected:
    void mouse_press(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            dragging = true;
            p1 = p2 = event.scene_pos;
            rect = QRectF(p1, p2);
        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( dragging )
        {
            p2 = event.scene_pos;
            update_rect(event.modifiers());
            event.repaint();
        }
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton && dragging )
        {
            dragging = false;
            auto shape = std::make_unique<model::Rect>(event.window->document());
            shape->position.set(rect.center());
            shape->size.set(rect.size());
            create_shape(QObject::tr("Draw Rectangle"), event, std::move(shape));
        }
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void key_press(const KeyEvent& event) override
    {
        if ( dragging )
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
        /// \todo Parent node transforms
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

    bool dragging = false;
    QPointF p1;
    QPointF p2;
    QRectF rect;

    static Autoreg<RectangleTool> autoreg;
};

} // namespace tools
