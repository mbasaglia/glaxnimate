#include "draw_tool_base.hpp"
#include "model/shapes/path.hpp"
#include "math/bezier/operations.hpp"

namespace tools {

class FreehandTool : public DrawToolBase
{
public:
    QString id() const override { return "draw-freehand"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-freehand"); }
    QString name() const override { return QObject::tr("Draw Freehand"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F6"), QKeySequence::PortableText); }

    void mouse_press(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            path.add_point(event.scene_pos);
        }
    }
    void mouse_move(const MouseEvent& event) override
    {
        if ( !path.empty() )
        {
            path.add_point(event.scene_pos);
            event.repaint();
        }

    }
    void mouse_release(const MouseEvent& event) override
    {
        if ( path.size() > 1 )
        {
            auto shape = std::make_unique<model::Path>(event.window->document());
            math::bezier::simplify(path, 128);
            shape->shape.set(path);
            path.clear();
            create_shape(QObject::tr("Draw Freehand"), event, std::move(shape));
        }
        else
        {
            if ( path.size() == 1 )
                path.clear();
            edit_clicked(event);
        }
    }

    void mouse_double_click(const MouseEvent& event) override
    {
        edit_clicked(event);
    }

    void paint(const PaintEvent& event) override
    {
        if ( !path.empty() )
        {
            QPainterPath ppath;
            path.add_to_painter_path(ppath);
            draw_shape(event, event.view->mapFromScene(ppath));
        }
    }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape )
        {
            path.clear();
            event.repaint();
            event.accept();
        }
    }

    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

private:
    static Autoreg<FreehandTool> autoreg;
    math::bezier::Bezier path;
};


} // namespace tools


tools::Autoreg<tools::FreehandTool> tools::FreehandTool::autoreg{tools::Registry::Draw, max_priority + 1};
