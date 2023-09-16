/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "draw_tool.hpp"

#include <QShortcut>

#include "model/shapes/path.hpp"
#include "app/settings/keyboard_shortcuts.hpp"
#include "glaxnimate_app.hpp"
#include "math/geom.hpp"
#include "math/vector.hpp"

class glaxnimate::gui::tools::DrawTool::Private
{
public:
    class ToolUndoStack
    {
    public:

        void push(QUndoCommand* command, SelectionManager* window)
        {
            if ( !undo_stack )
            {
                undo_stack = std::make_unique<QUndoStack>();
                window->undo_group().addStack(undo_stack.get());
                window->undo_group().setActiveStack(undo_stack.get());
            }

            undo_stack->push(command);
        }

        void undo(SelectionManager* window)
        {
            if ( undo_stack )
            {
                if ( undo_stack->index() == 0 )
                    clear(window);
                else
                    undo_stack->undo();
            }
        }

        void redo(SelectionManager*)
        {
            if ( undo_stack )
                undo_stack->redo();
        }

        void clear(SelectionManager* window)
        {
            if ( undo_stack )
            {
                window->undo_group().removeStack(undo_stack.get());
                window->undo_group().setActiveStack(&window->document()->undo_stack());
                undo_stack.reset();
            }
        }

    private:
        std::unique_ptr<QUndoStack> undo_stack;
    };

    class BezierUndoCommand : public QUndoCommand
    {
    public:
        BezierUndoCommand(const QString& name, math::bezier::Bezier* target, math::bezier::Bezier&& after )
            : QUndoCommand(name),
              target(target),
              before(*target),
              after(std::move(after))
        {}

        void undo() override
        {
            *target = before;
        }

        void redo() override
        {
            *target = after;
        }

        math::bezier::Bezier* target;
        math::bezier::Bezier before;
        math::bezier::Bezier after;
    };

    void create(const Event& event, DrawTool* tool);
    void adjust_point_type(Qt::KeyboardModifiers mod);
    void clear(bool hard, SelectionManager* window);
    void add_extension_points(model::Path* owner);
    void remove_extension_points(model::AnimatedProperty<math::bezier::Bezier>* property);
    void recursive_add_selection(graphics::DocumentScene * scene, model::VisualNode* node);
    void recursive_remove_selection(graphics::DocumentScene * scene, model::VisualNode* node);
    bool within_join_distance(const glaxnimate::gui::tools::MouseEvent& event, const QPointF& scene_pos);
    void prepare_draw(const glaxnimate::gui::tools::MouseEvent& event);

    void push_command(const QString& name, SelectionManager* window, math::bezier::Bezier&& after)
    {
        undo_stack.push(new BezierUndoCommand(name, &bezier, std::move(after)), window);
    }

    struct ExtendPathData
    {
        model::AnimatedProperty<math::bezier::Bezier>* property = nullptr;
        model::Path* owner = nullptr;
        bool at_end = true;

        QPointF pos() const
        {
            QPointF p = at_end ? property->get().back().pos : property->get()[0].pos;
            if ( owner )
                p = owner->transform_matrix(owner->time()).map(p);
            return p;
        }
    };

    QPointF best_point(const glaxnimate::gui::tools::MouseEvent& event)
    {
        if ( bezier.size() < 2 || !(event.modifiers() & Qt::ControlModifier) )
            return event.scene_pos;

        const QPointF& ref = bezier.points()[bezier.size() - 2].pos;
        QPointF best = math::line_closest_point(ref, ref + QPointF(10, 0), event.scene_pos);
        auto best_dist = math::distance(best, event.scene_pos);
        static const std::array<QPointF, 3> offsets{QPointF(0, 10), QPointF(10, 10), QPointF(10, -10)};
        for ( const auto& off : offsets )
        {
            QPointF p = math::line_closest_point(ref, ref + off, event.scene_pos);
            auto dist = math::distance(p, event.scene_pos);
            if ( dist < best_dist )
            {
                best = p;
                best_dist = dist;
            }
        }

        return best;
    }

    math::bezier::Bezier bezier;
    bool dragging = false;
    math::bezier::PointType point_type = math::bezier::Symmetrical;
    qreal join_radius = 5;
    bool joining = false;

    ExtendPathData extend;
    std::vector<ExtendPathData> extension_points;

    ToolUndoStack undo_stack;
};

glaxnimate::gui::tools::Autoreg<glaxnimate::gui::tools::DrawTool> glaxnimate::gui::tools::DrawTool::autoreg{max_priority};

void glaxnimate::gui::tools::DrawTool::Private::create(const glaxnimate::gui::tools::Event& event, DrawTool* tool)
{
    if ( !bezier.empty() )
    {
#ifdef Q_OS_ANDROID
        if ( bezier.closed() )
#endif
        bezier.points().pop_back();


        // use symmetrical length while drawing but for editing having smooth nodes is nicer
        // the user can always change them back
        for ( auto & point : bezier )
        {
            if ( point.type == math::bezier::PointType::Symmetrical )
            {
                if ( math::fuzzy_compare(point.pos, point.tan_in) && math::fuzzy_compare(point.pos, point.tan_out) )
                    point.type = math::bezier::PointType::Corner;
                else
                    point.type = math::bezier::PointType::Smooth;
            }
        }

        if ( extend.property )
        {
            command::UndoMacroGuard guard(tr("Extend Path"), event.window->document());

            if ( !extend.at_end )
                bezier.reverse();

            extend.property->extend(bezier, extend.at_end);

            if ( bezier.closed() )
            {
                if ( extend.owner )
                    extend.owner->closed.set_undoable(true);
                remove_extension_points(extend.property);
            }
        }
        else
        {
            auto shape = std::make_unique<model::Path>(event.window->document());
            shape->shape.set(bezier);
            if ( bezier.closed() )
                shape->closed.set(true);

            add_extension_points(shape.get());

            tool->create_shape(tr("Draw Shape"), event, std::move(shape));
        }

    }
    clear(false, event.window);
    event.repaint();
}

void glaxnimate::gui::tools::DrawTool::Private::add_extension_points(model::Path* owner)
{
    if ( !owner->closed.get() )
    {
        extension_points.push_back(ExtendPathData{
            &owner->shape,
            owner,
            true
        });
        extension_points.push_back(ExtendPathData{
            &owner->shape,
            owner,
            false
        });
    }
}

void glaxnimate::gui::tools::DrawTool::Private::remove_extension_points(model::AnimatedProperty<math::bezier::Bezier>* property)
{
    extension_points.erase(
        std::remove_if(extension_points.begin(), extension_points.end(), [property](const ExtendPathData& p){
            return p.property == property;
        }),
        extension_points.end()
    );
}

void glaxnimate::gui::tools::DrawTool::Private::adjust_point_type(Qt::KeyboardModifiers mod)
{
    if ( mod & Qt::ShiftModifier )
        point_type = math::bezier::Corner;
    else if ( mod & Qt::ControlModifier )
        point_type = math::bezier::Smooth;
    else
        point_type = math::bezier::Symmetrical;

    if ( !bezier.empty() )
    {
        bezier.points().back().type = point_type;
//         bezier.points().back().drag_tan_out(bezier.points().back().tan_out);
    }
}

void glaxnimate::gui::tools::DrawTool::key_press(const glaxnimate::gui::tools::KeyEvent& event)
{
    if ( d->bezier.empty() )
        return;

    if ( event.key() == Qt::Key_Delete || event.key() == Qt::Key_Backspace || event.key() == Qt::Key_Back )
    {
        remove_last(event.window);
        event.accept();
        event.repaint();
    }
    else if ( event.key() == Qt::Key_Shift || event.key() == Qt::Key_Control )
    {
        d->adjust_point_type(event.modifiers());
        event.accept();
        event.repaint();
    }
    else if ( event.key() == Qt::Key_Enter || event.key() == Qt::Key_Return )
    {
        d->create(event, this);
        event.accept();
    }
    else if ( event.key() == Qt::Key_Escape )
    {
        d->clear(false, event.window);
        event.repaint();
    }
}

void glaxnimate::gui::tools::DrawTool::Private::clear(bool hard, SelectionManager* window)
{
    undo_stack.clear(window);
    dragging = false;
    bezier.clear();
    point_type = math::bezier::Symmetrical;
    joining = false;
    extend = {};
    if ( hard )
        extension_points.clear();
}

void glaxnimate::gui::tools::DrawTool::remove_last(SelectionManager* window)
{
    if ( d->bezier.empty() )
        return;

#ifndef Q_OS_ANDROID
    int back = 2;
#else
    int back = 1;
#endif

    if ( d->bezier.size() <= back )
    {
        d->clear(false, window);
    }
    else
    {
        auto bezier = d->bezier;
        bezier.points().erase(bezier.points().end() - back);
        d->push_command(tr("Delete curve point"), window, std::move(bezier));
    }

}


bool glaxnimate::gui::tools::DrawTool::Private::within_join_distance(const glaxnimate::gui::tools::MouseEvent& event, const QPointF& scene_pos)
{
    return math::length(event.pos() - event.view->mapFromScene(scene_pos)) <= join_radius;
}

void glaxnimate::gui::tools::DrawTool::Private::prepare_draw(const glaxnimate::gui::tools::MouseEvent& event)
{
    for ( const auto& point : extension_points )
    {
        if ( within_join_distance(event, point.pos()) )
        {
            extend = point;
            bezier = point.property->get();
            if ( !point.at_end )
                bezier.reverse();
            bezier.points().back().type = math::bezier::Corner;
            bezier.points()[0].type = math::bezier::Corner;
            return;
        }
    }

    bezier.push_back(math::bezier::Point(event.scene_pos, event.scene_pos, event.scene_pos, math::bezier::Corner));
}

glaxnimate::gui::tools::DrawTool::DrawTool()
    : d(std::make_unique<Private>())
{
}

glaxnimate::gui::tools::DrawTool::~DrawTool()
{
}

void glaxnimate::gui::tools::DrawTool::mouse_press(const glaxnimate::gui::tools::MouseEvent& event)
{
    if ( event.button() != Qt::LeftButton )
        return;

    if ( d->bezier.empty() )
    {
        d->prepare_draw(event);
    }
#ifdef Q_OS_ANDROID
    else
    {
        QPointF pos = d->best_point(event);
        d->bezier.points().push_back(math::bezier::Point(pos, pos, pos, d->point_type));
        event.repaint();
    }
#endif

    d->dragging = true;

}

void glaxnimate::gui::tools::DrawTool::mouse_move(const glaxnimate::gui::tools::MouseEvent& event)
{
    if ( d->bezier.empty() )
        return;

    if ( d->dragging )
    {
        d->bezier.points().back().drag_tan_out(event.scene_pos);
    }
    else if ( d->bezier.size() > 2 && d->within_join_distance(event, d->bezier.points().front().pos) )
    {
        d->joining = true;
#ifndef Q_OS_ANDROID
        d->bezier.points().back().translate_to(d->bezier.points().front().pos);
#endif
    }
    else
    {
        d->joining = false;
#ifndef Q_OS_ANDROID
        d->bezier.points().back().translate_to(d->best_point(event));
#endif
    }
}

void glaxnimate::gui::tools::DrawTool::mouse_release(const glaxnimate::gui::tools::MouseEvent& event)
{
    if ( !d->dragging )
        return;

    if ( event.button() == Qt::LeftButton )
    {
        d->dragging = false;

        if ( d->joining )
        {
            d->bezier.points().front().tan_in = d->bezier.points().back().tan_in;
            d->bezier.set_closed(true);
            d->create(event, this);
        }
#ifndef Q_OS_ANDROID
        else
        {
            auto bezier = d->bezier;
            QPointF pos = d->best_point(event);
            bezier.points().push_back(math::bezier::Point(pos, pos, pos, d->point_type));
            d->push_command(tr("Add curve point"), event.window, std::move(bezier));
            event.repaint();
        }
#endif
    }
}

void glaxnimate::gui::tools::DrawTool::mouse_double_click(const glaxnimate::gui::tools::MouseEvent& event)
{
    d->create(event, this);
    event.accept();
}

void glaxnimate::gui::tools::DrawTool::paint(const glaxnimate::gui::tools::PaintEvent& event)
{
    QPen pen(event.palette.highlight(), 1);
    pen.setCosmetic(true);
    qreal view_radius = d->join_radius;

    if ( !d->bezier.empty() )
    {
        QPainterPath path;
        d->bezier.add_to_painter_path(path);
        draw_shape(event, event.view->mapFromScene(path));

        event.painter->setPen(pen);
        event.painter->setBrush(Qt::NoBrush);

#ifdef Q_OS_ANDROID
        if ( d->bezier.size() >= 1 )
#else
        if ( d->bezier.size() > 1 )
#endif
        {
            QPointF center = event.view->mapFromScene(d->bezier.points().front().pos);

            if ( d->joining )
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

#ifndef Q_OS_ANDROID
        if ( d->dragging )
#endif
        {
            pen.setWidth(2);
            event.painter->setPen(pen);

            QPolygonF poly;
            if ( d->bezier.size() > 1 )
                poly.push_back(event.view->mapFromScene(d->bezier.points().back().tan_in));
            QPointF center = event.view->mapFromScene(d->bezier.points().back().pos);
            poly.push_back(center);
            poly.push_back(event.view->mapFromScene(d->bezier.points().back().tan_out));
            event.painter->drawPolyline(poly);
            event.painter->drawEllipse(center, view_radius, view_radius);
        }
    }
    else
    {
        event.painter->setPen(pen);
        event.painter->setBrush(event.palette.brush(QPalette::Active, QPalette::HighlightedText));
        bool got_one = false;
        for ( const auto& point : d->extension_points )
        {
            QPointF center = event.view->mapFromScene(point.pos());
            qreal mult = 1;
            if ( !got_one && math::length(event.view->mapFromGlobal(QCursor::pos()) - center) <= d->join_radius )
            {
                mult = 1.5;
                got_one = true;
            }
            event.painter->drawEllipse(center, view_radius * mult, view_radius * mult);
        }
    }
}

void glaxnimate::gui::tools::DrawTool::enable_event(const Event& ev)
{
    d->clear(false, ev.window);
    ev.repaint();
}

void glaxnimate::gui::tools::DrawTool::disable_event(const Event& event)
{
    d->create(event, this);
    d->clear(true, event.window);
}

void glaxnimate::gui::tools::DrawTool::on_selected(graphics::DocumentScene * scene, model::VisualNode * node)
{
    d->recursive_add_selection(scene, node);
}

void glaxnimate::gui::tools::DrawTool::on_deselected(graphics::DocumentScene * scene, model::VisualNode* node)
{
    d->recursive_remove_selection(scene, node);
}

void glaxnimate::gui::tools::DrawTool::Private::recursive_add_selection(graphics::DocumentScene * scene, model::VisualNode* node)
{
    auto meta = node->metaObject();
    if ( meta->inherits(&model::Path::staticMetaObject) )
    {
        add_extension_points(static_cast<model::Path*>(node));
    }
    else if ( meta->inherits(&model::Group::staticMetaObject) )
    {
        for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
            recursive_add_selection(scene, sub.get());
    }
}

void glaxnimate::gui::tools::DrawTool::Private::recursive_remove_selection(graphics::DocumentScene * scene, model::VisualNode* node)
{
    auto meta = node->metaObject();
    if ( meta->inherits(&model::Path::staticMetaObject) )
    {
        remove_extension_points(&static_cast<model::Path*>(node)->shape);
    }
    else if ( meta->inherits(&model::Group::staticMetaObject) )
    {
        for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
            recursive_remove_selection(scene, sub.get());
    }
}

void glaxnimate::gui::tools::DrawTool::initialize(const Event& event)
{
    d->join_radius = 5 * GlaxnimateApp::handle_size_multiplier();
    Q_UNUSED(event);
}
