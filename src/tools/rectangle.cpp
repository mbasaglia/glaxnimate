#include "base.hpp"

#include "ui/widgets/tools/shape_tool_widget.hpp"

namespace tools {

class RectangleTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-rectangle"); }
    QString name() const override { return QObject::tr("Rectangle"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F4"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

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
            /// \todo Create rect
        }
    }

    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void paint(const PaintEvent& event) override
    {
        /// \todo Parent node transforms
        if ( dragging )
        {
            ShapeToolWidget* options = widget();

            QPolygon rect = event.view->mapFromScene(QRectF(p1, p2));

            if ( !options->create_stroke() && !options->create_fill() )
            {
                event.painter->setBrush(Qt::transparent);

//                 event.painter->setCompositionMode(QPainter::CompositionMode_Difference);
                QPen p(Qt::white, 1);
                p.setCosmetic(true);
                p.setDashPattern({4., 4.});
                event.painter->setPen(p);
                event.painter->drawPolygon(rect);

//                 event.painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                p.setDashOffset(4);
                p.setColor(Qt::black);
                event.painter->setPen(p);
                event.painter->drawPolygon(rect);
            }
            else
            {
                if ( options->create_fill() )
                    event.painter->setBrush(event.window->current_color());
                else
                    event.painter->setBrush(Qt::transparent);

                if ( options->create_stroke() )
                    event.painter->setPen(QPen(event.window->secondary_color(), options->stroke_width()));
                else
                    event.painter->setPen(QPen(Qt::transparent, 0));

                event.painter->drawPolygon(rect);
            }
        }
    }

    QWidget* on_create_widget() override
    {
        return new ShapeToolWidget();
    }

    QCursor cursor() override { return Qt::CrossCursor; }

private:
    ShapeToolWidget* widget()
    {
        return static_cast<ShapeToolWidget*>(get_settings_widget());
    }

    bool dragging = false;
    QPointF p1;
    QPointF p2;

    static Autoreg<RectangleTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::RectangleTool> tools::RectangleTool::autoreg{tools::Registry::Shape, max_priority};
