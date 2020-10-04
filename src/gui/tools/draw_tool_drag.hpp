#include "draw_tool_base.hpp"

namespace tools {

/**
 * \brief Base class for drawing tools where you click and drag to draw the shape
 */
class DrawToolDrag : public DrawToolBase
{
protected:
    void mouse_press(const MouseEvent& event) override
    {
        if ( event.button() == Qt::LeftButton )
        {
            dragging = false;
            p1 = p2 = event.scene_pos;
            on_drag_start();
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
            on_drag(event);
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
                on_drag_complete(event);
                event.repaint();
            }
            else
            {
                edit_clicked(event);
            }
        }
    }

    void mouse_double_click(const MouseEvent& event) override
    {
        edit_clicked(event);
    }
    void key_release(const KeyEvent& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    bool show_editors(model::DocumentNode*) const override { return false; }

    virtual void on_drag_start() = 0;
    virtual void on_drag(const MouseEvent& event) = 0;
    virtual void on_drag_complete(const MouseEvent& event) = 0;

protected:
    bool dragging = false;
    QPointF p1;
    QPointF p2;

};


} // namespace tools
