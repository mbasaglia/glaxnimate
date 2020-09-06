#include "draw_tool.hpp"

tools::Autoreg<tools::DrawTool> tools::DrawTool::autoreg{tools::Registry::Draw, max_priority};

void tools::DrawTool::create(const tools::Event& event)
{
    if ( item )
    {
        item->pop_back();
        auto shape = std::make_unique<model::Path>(event.window->document());
        shape->shape.set(item->bezier());
        item.reset();
        dragging = false;
        point_type = math::Smooth;
        create_shape(QObject::tr("Draw Shape"), event, std::move(shape));
        event.repaint();
    }
}

void tools::DrawTool::adjust_point_type(Qt::KeyboardModifiers mod)
{
    if ( mod & Qt::ShiftModifier )
        point_type = math::Corner;
    else if ( (mod & Qt::ControlModifier) && dragging )
        point_type = math::Smooth;
    else
        point_type = math::Symmetrical;

    if ( dragging && item->size() )
        item->set_type(item->size()-1, point_type);
}


void tools::DrawTool::key_press(const tools::KeyEvent& event)
{
    if ( !item )
        return;

    if ( event.key() == Qt::Key_Delete || event.key() == Qt::Key_Backspace )
    {
        item->pop_back();
        event.accept();
    }
    else if ( event.key() == Qt::Key_Shift )
    {
        adjust_point_type(event.modifiers());
        event.accept();
    }
    else if ( event.key() == Qt::Key_Control && dragging )
    {
        adjust_point_type(event.modifiers());
        event.accept();
    }
    else if ( event.key() == Qt::Key_Enter || event.key() == Qt::Key_Return )
    {
        create(event);
        event.accept();
    }
    else if ( event.key() == Qt::Key_Escape )
    {
        dragging = false;
        item.reset();
        event.repaint();
    }
}

void tools::DrawTool::key_release(const tools::KeyEvent& event)
{
    if ( !item )
        return;

    if ( event.key() == Qt::Key_Shift && point_type == math::Corner )
    {
        adjust_point_type(event.modifiers());
    }
}

void tools::DrawTool::mouse_press(const tools::MouseEvent& event)
{
    if ( event.button() != Qt::LeftButton )
        return;

    if ( !item )
    {
        item = std::make_unique<graphics::BezierItem>();
        item->add_point(event.scene_pos, point_type);
        event.scene->addItem(item.get());
    }
    dragging = true;
}

void tools::DrawTool::mouse_move(const tools::MouseEvent& event)
{
    if ( !item )
        return;

    if ( dragging )
        item->drag_tan_out(event.scene_pos);
    else
        item->drag_pos(event.scene_pos);
}

void tools::DrawTool::mouse_release(const tools::MouseEvent& event)
{
    if ( !dragging )
        return;

    if ( event.button() == Qt::LeftButton )
    {
        dragging = false;
        item->add_point(event.scene_pos, point_type);
    }
}

void tools::DrawTool::mouse_double_click(const tools::MouseEvent& event)
{
    create(event);
    event.accept();
}

void tools::DrawTool::paint(const tools::PaintEvent& event)
{
    if ( item )
    {
        QPainterPath path;
        item->bezier().add_to_painter_path(path);
        draw_shape(event, event.view->mapFromScene(path));
    }
}

bool tools::DrawTool::show_editors(model::DocumentNode*) const
{
    return false;
}







