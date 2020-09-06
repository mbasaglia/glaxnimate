#pragma once

#include "draw_tool_base.hpp"
#include "graphics/bezier_item.hpp"

namespace tools {


class DrawTool : public DrawToolBase
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-path"); }
    QString name() const override { return QObject::tr("Draw Bezier"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F3"), QKeySequence::PortableText); }

    void mouse_press(const MouseEvent& event) override;
    void mouse_move(const MouseEvent& event) override;
    void mouse_release(const MouseEvent& event) override;
    void mouse_double_click(const MouseEvent& event) override;
    void paint(const PaintEvent& event) override;
    void key_press(const KeyEvent& event) override;
    void key_release(const KeyEvent& event) override;
    bool show_editors(model::DocumentNode* node) const override;
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

private:
    void create(const Event& event);
    void adjust_point_type(Qt::KeyboardModifiers mod);

    std::unique_ptr<graphics::BezierItem> item;
    bool dragging = false;
    math::BezierPointType point_type = math::Symmetrical;
    static Autoreg<DrawTool> autoreg;
};

} // namespace tools
