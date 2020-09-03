#include "rectangle.hpp"
#include "model/shapes/ellipse.hpp"

namespace tools {

class EllipseTool : public RectangleTool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-ellipse"); }
    QString name() const override { return QObject::tr("Ellipse"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F5"), QKeySequence::PortableText); }

protected:
    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton && dragging )
        {
            dragging = false;
            auto shape = std::make_unique<model::Ellipse>(event.window->document());
            shape->position.set(rect.center());
            shape->size.set(rect.size());
            create_shape(QObject::tr("Draw Ellipse"), event, std::move(shape));
            event.repaint();
        }
    }

    void paint(const PaintEvent& event) override
    {
        /// \todo Parent node transforms
        if ( dragging )
        {
            QPainterPath path;
            path.addEllipse(rect);
            path = event.view->mapFromScene(path);
            draw_shape(event, path);
        }
    }

private:
    static Autoreg<EllipseTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EllipseTool> tools::EllipseTool::autoreg{tools::Registry::Shape, max_priority + 1};
