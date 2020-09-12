#include "draw_tool.hpp"
#include "model/shapes/path.hpp"


tools::Autoreg<tools::DrawTool> tools::DrawTool::autoreg{tools::Registry::Draw, max_priority};

void tools::DrawTool::create(const tools::Event& event)
{
    if ( !bezier.empty() )
    {
        bezier.points().pop_back();
        auto shape = std::make_unique<model::Path>(event.window->document());
        // use symmetrical length while drawing but for editing having smooth nodes is nicer
        // the user can always change them back
        for ( auto & point : bezier )
        {
            if ( point.type == math::BezierPointType::Symmetrical )
                point.type = math::BezierPointType::Smooth;
        }
        shape->shape.set(bezier);
        clear();
        create_shape(QObject::tr("Draw Shape"), event, std::move(shape));
        event.repaint();
    }
}

void tools::DrawTool::adjust_point_type(Qt::KeyboardModifiers mod)
{
    if ( mod & Qt::ShiftModifier )
        point_type = math::Corner;
    else if ( mod & Qt::ControlModifier )
        point_type = math::Smooth;
    else
        point_type = math::Symmetrical;

    if ( !bezier.empty() )
        bezier.points().back().type = point_type;
}

void tools::DrawTool::key_press(const tools::KeyEvent& event)
{
    if ( bezier.empty() )
        return;

    if ( event.key() == Qt::Key_Delete || event.key() == Qt::Key_Backspace )
    {
        bezier.points().pop_back();
        bezier.empty();
        clear();
        event.accept();
        event.repaint();
    }
    else if ( event.key() == Qt::Key_Shift || event.key() == Qt::Key_Control )
    {
        adjust_point_type(event.modifiers());
        event.accept();
        event.repaint();
    }
    else if ( event.key() == Qt::Key_Enter || event.key() == Qt::Key_Return )
    {
        create(event);
        event.accept();
    }
    else if ( event.key() == Qt::Key_Escape )
    {
        clear();
        event.repaint();
    }
}

void tools::DrawTool::clear()
{
    dragging = false;
    bezier.clear();
    point_type = math::Symmetrical;
    joining = false;
}

void tools::DrawTool::key_release(const tools::KeyEvent& event)
{
    if ( bezier.empty() )
        return;

    if ( event.key() == Qt::Key_Shift || event.key() == Qt::Key_Control )
    {
        adjust_point_type(event.modifiers());
        event.accept();
        event.repaint();
    }
}

void tools::DrawTool::mouse_press(const tools::MouseEvent& event)
{
    if ( event.button() != Qt::LeftButton )
        return;

    if ( bezier.empty() )
        bezier.push_back(math::BezierPoint(event.scene_pos, event.scene_pos, event.scene_pos, math::Corner));

    dragging = true;
}

void tools::DrawTool::mouse_move(const tools::MouseEvent& event)
{
    if ( bezier.empty() )
        return;

    if ( dragging )
    {
        bezier.points().back().drag_tan_out(event.scene_pos);
    }
    else if ( bezier.size() > 2 && math::length(event.pos() - event.view->mapFromScene(bezier.points().front().pos)) <= join_radius )
    {
        joining = true;
        bezier.points().back().translate_to(bezier.points().front().pos);
    }
    else
    {
        joining = false;
        bezier.points().back().translate_to(event.scene_pos);
    }
}

void tools::DrawTool::mouse_release(const tools::MouseEvent& event)
{
    if ( !dragging )
        return;

    if ( event.button() == Qt::LeftButton )
    {
        dragging = false;

        if ( joining )
        {
            bezier.points().front().tan_in = bezier.points().back().tan_in;
            bezier.set_closed(true);
            create(event);
        }
        else
        {
            bezier.push_back(math::BezierPoint(event.scene_pos, event.scene_pos, event.scene_pos, point_type));
            event.repaint();
        }
    }
}

void tools::DrawTool::mouse_double_click(const tools::MouseEvent& event)
{
    create(event);
    event.accept();
}

void tools::DrawTool::paint(const tools::PaintEvent& event)
{
    if ( !bezier.empty() )
    {
        QPainterPath path;
        bezier.add_to_painter_path(path);
        draw_shape(event, event.view->mapFromScene(path));

        QPen pen(event.palette.highlight(), 1);
        pen.setCosmetic(true);
        event.painter->setPen(pen);
        event.painter->setBrush(Qt::NoBrush);

        qreal view_radius = join_radius / event.view->get_zoom_factor();
        if ( bezier.size() > 1 )
        {
            QPointF center = event.view->mapFromScene(bezier.points().front().pos);

            if ( joining )
            {
                event.painter->setBrush(event.palette.highlightedText());
                event.painter->drawEllipse(center, view_radius*1.5, view_radius*1.5);
                event.painter->setBrush(Qt::NoBrush);
            }
            else
            {
                event.painter->drawEllipse(center, view_radius, view_radius);
            }
        }

        if ( dragging )
        {
            QPolygonF poly;
            if ( bezier.size() > 1 )
                poly.push_back(event.view->mapFromScene(bezier.points().back().tan_in));
            QPointF center = event.view->mapFromScene(bezier.points().back().pos);
            poly.push_back(center);
            poly.push_back(event.view->mapFromScene(bezier.points().back().tan_out));
            event.painter->drawPolyline(poly);
            event.painter->drawEllipse(center, view_radius, view_radius);
        }
    }
}

bool tools::DrawTool::show_editors(model::DocumentNode*) const
{
    return false;
}







