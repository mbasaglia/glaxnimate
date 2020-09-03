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
        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( dragging )
        {
            p2 = event.scene_pos;
        }
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton && dragging )
        {
            dragging = false;
            auto shape = std::make_unique<model::Rect>(event.window->document());
            shape->position.set((p1 + p2) / 2);
            QPointF sz = p2 - p1;
            shape->size.set(QSizeF(sz.x(), sz.y()));
            create_shape(QObject::tr("Draw Rectangle"), event, std::move(shape));
        }
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void paint(const PaintEvent& event) override
    {
        /// \todo Parent node transforms
        if ( dragging )
        {
            QPainterPath path;
            path.addPolygon(event.view->mapFromScene(QRectF(p1, p2)));
            path.closeSubpath();
            draw_shape(event, path);
        }
    }

private:
    bool dragging = false;
    QPointF p1;
    QPointF p2;

    static Autoreg<RectangleTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::RectangleTool> tools::RectangleTool::autoreg{tools::Registry::Shape, max_priority};
