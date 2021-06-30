#include "draw_tool.hpp"

#include <QShortcut>

#include "model/shapes/path.hpp"
#include "app/settings/keyboard_shortcuts.hpp"
#include "glaxnimate_app.hpp"

class tools::DrawTool::Private
{
public:
    void create(const Event& event, DrawTool* tool);
    void adjust_point_type(Qt::KeyboardModifiers mod);
    void clear(bool hard);
    void add_extension_points(model::Path* owner);
    void remove_extension_points(model::AnimatedProperty<math::bezier::Bezier>* property);
    void recursive_add_selection(graphics::DocumentScene * scene, model::VisualNode* node);
    void recursive_remove_selection(graphics::DocumentScene * scene, model::VisualNode* node);
    bool within_join_distance(const tools::MouseEvent& event, const QPointF& scene_pos);
    void prepare_draw(const tools::MouseEvent& event);

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

    math::bezier::Bezier bezier;
    bool dragging = false;
    math::bezier::PointType point_type = math::bezier::Symmetrical;
    qreal join_radius = 5;
    bool joining = false;

    ExtendPathData extend;
    std::vector<ExtendPathData> extension_points;

    QShortcut* undo = nullptr;
    // For some reason there's an annoying dialog if a QShortcut has the same
    // key sequence as a QAction, so we work around it...
    QAction* why_cant_we_have_nice_things = nullptr;
};

tools::Autoreg<tools::DrawTool> tools::DrawTool::autoreg{max_priority};

void tools::DrawTool::Private::create(const tools::Event& event, DrawTool* tool)
{
    if ( !bezier.empty() )
    {
        bezier.points().pop_back();


        // use symmetrical length while drawing but for editing having smooth nodes is nicer
        // the user can always change them back
        for ( auto & point : bezier )
        {
            if ( point.type == math::bezier::PointType::Symmetrical )
                point.type = math::bezier::PointType::Smooth;
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
    clear(false);
    event.repaint();
}

void tools::DrawTool::Private::add_extension_points(model::Path* owner)
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

void tools::DrawTool::Private::remove_extension_points(model::AnimatedProperty<math::bezier::Bezier>* property)
{
    extension_points.erase(
        std::remove_if(extension_points.begin(), extension_points.end(), [property](const ExtendPathData& p){
            return p.property == property;
        }),
        extension_points.end()
    );
}

void tools::DrawTool::Private::adjust_point_type(Qt::KeyboardModifiers mod)
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
        bezier.points().back().drag_tan_out(bezier.points().back().tan_out);
    }
}

void tools::DrawTool::key_press(const tools::KeyEvent& event)
{
    if ( d->bezier.empty() )
        return;

    if ( event.key() == Qt::Key_Delete || event.key() == Qt::Key_Backspace )
    {
        remove_last();
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
        d->clear(false);
        event.repaint();
    }
}

void tools::DrawTool::Private::clear(bool hard)
{
#ifndef Q_OS_ANDROID
    undo->setEnabled(false);
    why_cant_we_have_nice_things->setEnabled(true);
#endif
    dragging = false;
    bezier.clear();
    point_type = math::bezier::Symmetrical;
    joining = false;
    extend = {};
    if ( hard )
        extension_points.clear();
}

void tools::DrawTool::remove_last()
{
    if ( d->bezier.empty() )
        return;

    if ( d->bezier.size() <= 2 )
        d->clear(false);
    else
        d->bezier.points().erase(d->bezier.points().end() - 2);
}


bool tools::DrawTool::Private::within_join_distance(const tools::MouseEvent& event, const QPointF& scene_pos)
{
    return math::length(event.pos() - event.view->mapFromScene(scene_pos)) <= join_radius;
}

void tools::DrawTool::Private::prepare_draw(const tools::MouseEvent& event)
{
#ifndef Q_OS_ANDROID
    undo->setEnabled(true);
    why_cant_we_have_nice_things->setEnabled(false);
#endif

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

tools::DrawTool::DrawTool()
    : d(std::make_unique<Private>())
{
}

tools::DrawTool::~DrawTool()
{
}

void tools::DrawTool::key_release(const tools::KeyEvent& event)
{
    if ( d->bezier.empty() )
        return;

    if ( event.key() == Qt::Key_Shift || event.key() == Qt::Key_Control )
    {
        d->adjust_point_type(event.modifiers());
        event.accept();
        event.repaint();
    }
}

void tools::DrawTool::mouse_press(const tools::MouseEvent& event)
{
    if ( event.button() != Qt::LeftButton )
        return;

    if ( d->bezier.empty() )
        d->prepare_draw(event);

    d->dragging = true;
}

void tools::DrawTool::mouse_move(const tools::MouseEvent& event)
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
        d->bezier.points().back().translate_to(d->bezier.points().front().pos);
    }
    else
    {
        d->joining = false;
        d->bezier.points().back().translate_to(event.scene_pos);
    }
}

void tools::DrawTool::mouse_release(const tools::MouseEvent& event)
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
        else
        {
            d->bezier.push_back(math::bezier::Point(event.scene_pos, event.scene_pos, event.scene_pos, d->point_type));
            event.repaint();
        }
    }
}

void tools::DrawTool::mouse_double_click(const tools::MouseEvent& event)
{
    d->create(event, this);
    event.accept();
}

void tools::DrawTool::paint(const tools::PaintEvent& event)
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

        if ( d->bezier.size() > 1 )
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

        if ( d->dragging )
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

void tools::DrawTool::enable_event(const Event& ev)
{
    d->clear(false);
    ev.repaint();
}

void tools::DrawTool::disable_event(const Event&)
{
    d->clear(true);
}

void tools::DrawTool::on_selected(graphics::DocumentScene * scene, model::VisualNode * node)
{
    d->recursive_add_selection(scene, node);
}

void tools::DrawTool::on_deselected(graphics::DocumentScene * scene, model::VisualNode* node)
{
    d->recursive_remove_selection(scene, node);
}

void tools::DrawTool::Private::recursive_add_selection(graphics::DocumentScene * scene, model::VisualNode* node)
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

void tools::DrawTool::Private::recursive_remove_selection(graphics::DocumentScene * scene, model::VisualNode* node)
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

void tools::DrawTool::initialize(const Event& event)
{
#ifndef Q_OS_ANDROID
    d->undo = new QShortcut(GlaxnimateApp::instance()->shortcuts()->get_shortcut("action_undo"), event.view, nullptr, nullptr, Qt::WidgetShortcut);
    connect(d->undo, &QShortcut::activated, this, &DrawTool::remove_last);
    connect(d->undo, &QShortcut::activated, event.scene, [scene=event.scene]{ scene->update(); });
    d->why_cant_we_have_nice_things = GlaxnimateApp::instance()->shortcuts()->action("action_undo")->action;
    d->undo->setEnabled(false);
#endif
}
