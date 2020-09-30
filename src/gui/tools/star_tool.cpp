#include "draw_tool_base.hpp"
#include "model/shapes/polystar.hpp"
#include "math/math.hpp"
#include "widgets/tools/star_tool_widget.hpp"

namespace tools {

class StarTool : public DrawToolBase
{
public:
    QString id() const override { return "draw-star"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-polygon-star"); }
    QString name() const override { return QObject::tr("Star"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("*"), QKeySequence::PortableText); }

protected:
    void mouse_press(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            dragging = false;
            p1 = p2 = event.scene_pos;
            polar = {};
        }
    }

    void mouse_move(const MouseEvent& event) override
    {
        if ( !dragging && event.press_button == Qt::LeftButton )
        {
            dragging = true;
        }

        if ( dragging )
        {
            p2 = event.scene_pos;
            polar = p2 - p1;
            polar.angle = polar.angle + math::pi / 2;
            bez = model::PolyStar::draw(
                widget()->star_type(),
                p1,
                widget()->spoke_ratio() * polar.length,
                polar.length,
                polar.angle,
                widget()->points()
            );
            event.repaint();
        }
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            if ( dragging )
            {
                dragging = false;
                auto shape = std::make_unique<model::PolyStar>(event.window->document());
                shape->position.set(p1);
                shape->outer_radius.set(polar.length);
                shape->angle.set(math::rad2deg(polar.angle));
                shape->inner_radius.set(widget()->spoke_ratio() * polar.length);
                shape->type.set(widget()->star_type());
                shape->points.set(widget()->points());
                create_shape(QObject::tr("Draw Star"), event, std::move(shape));
                event.repaint();
                bez.clear();
            }
            else
            {
                check_click(event);
            }
        }
    }

    void paint(const PaintEvent& event) override
    {
        if ( dragging )
        {
            QPainterPath path;
            bez.add_to_painter_path(path);
            draw_shape(event, event.view->mapFromScene(path));
        }
    }

    QWidget* on_create_widget() override
    {
        return new StarToolWidget();
    }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape )
        {
            dragging = false;
            event.repaint();
            bez.clear();
            event.accept();
        }
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }
    QCursor cursor() override { return {}; }
    bool show_editors(model::DocumentNode* node) const override { Q_UNUSED(node); return false; }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

private:
    bool dragging = false;
    QPointF p1;
    QPointF p2;
    math::PolarVector<QPointF> polar;
    math::Bezier bez;


    StarToolWidget* widget()
    {
        return static_cast<StarToolWidget*>(get_settings_widget());
    }

    static Autoreg<StarTool> autoreg;
};


} // namespace tools

tools::Autoreg<tools::StarTool> tools::StarTool::autoreg{tools::Registry::Shape, max_priority + 2};

