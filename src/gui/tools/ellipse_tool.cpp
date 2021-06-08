#include "rectangle_tool.hpp"
#include "model/shapes/ellipse.hpp"

namespace tools {

class EllipseTool : public RectangleTool
{
public:
    QString id() const override { return "draw-ellipse"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-ellipse"); }
    QString name() const override { return QObject::tr("Ellipse"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F5"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Shape; }
    int group() const noexcept override { return static_group(); }

    void on_drag_complete(const MouseEvent& event) override
    {
        auto shape = std::make_unique<model::Ellipse>(event.window->document());
        rect = rect.normalized();
        shape->position.set(rect.center());
        shape->size.set(rect.size());
        create_shape(QObject::tr("Draw Ellipse"), event, std::move(shape));
    }

    void paint(const PaintEvent& event) override
    {
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

tools::Autoreg<tools::EllipseTool> tools::EllipseTool::autoreg{max_priority + 1};
