#include "path_modifier.hpp"

#include "model/shapes/group.hpp"
#include "model/shapes/path.hpp"

#include "model/animation/join_animatables.hpp"


static void to_path_value(
    const glaxnimate::math::bezier::MultiBezier& mbez,
    std::vector<glaxnimate::model::Path*>& paths,
    glaxnimate::model::Group* group,
    glaxnimate::model::Document* document
)
{
    for ( int i = 0; i < mbez.size(); i++ )
    {
        if ( i >= int(paths.size()) )
        {
            auto new_path = std::make_unique<glaxnimate::model::Path>(document);
            paths.push_back(new_path.get());
            group->shapes.insert(std::move(new_path));
        }

        auto path = paths[i];
        path->shape.set(mbez.beziers()[i]);
    }
}

static void to_path_frame(
    const glaxnimate::math::bezier::MultiBezier& mbez,
    std::vector<glaxnimate::model::Path*>& paths,
    glaxnimate::model::FrameTime t,
    const glaxnimate::model::KeyframeTransition& transition,
    glaxnimate::model::Group* group,
    glaxnimate::model::Document* document
)
{
    for ( int i = 0; i < mbez.size(); i++ )
    {
        if ( i >= int(paths.size()) )
        {
            auto new_path = std::make_unique<glaxnimate::model::Path>(document);
            if ( t > 0 )
            {
                glaxnimate::model::KeyframeTransition trans;
                trans.set_hold(true);
                new_path->shape.set_keyframe(0, glaxnimate::math::bezier::Bezier{})->set_transition({trans});
            }
            paths.push_back(new_path.get());
            group->shapes.insert(std::move(new_path));
        }

        auto path = paths[i];
        path->shape.set_keyframe(t, mbez.beziers()[i])->set_transition(transition);
    }
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::PathModifier::to_path() const
{
    auto group = std::make_unique<glaxnimate::model::Group>(document());
    group->name.set(name.get());
    group->group_color.set(group_color.get());
    group->visible.set(visible.get());


    std::vector<const AnimatableBase*> properties;
    auto flags = PropertyTraits::Visual|PropertyTraits::Animated;
    for ( auto prop : this->properties() )
    {
        if ( (prop->traits().flags & flags) == flags )
            properties.push_back(static_cast<AnimatableBase*>(prop));
    }

    JoinAnimatables ja(std::move(properties), JoinAnimatables::NoValues);
    FrameTime cur_time = ja.properties()[0]->time();

    std::vector<glaxnimate::model::Path*> paths;

    if ( ja.animated() )
    {
        for ( const auto & kf : ja )
        {
            auto bez = collect_shapes(kf.time, {});
            to_path_frame(bez, paths, kf.time, kf.transition(), group.get(), document());
        }
    }
    else
    {
        auto bez = collect_shapes(time(), {});
        to_path_value(bez, paths, group.get(), document());
    }

    group->set_time(cur_time);
    return group;

}

void glaxnimate::model::PathModifier::on_paint(QPainter* painter, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode mode, glaxnimate::model::Modifier*) const
{
    for ( auto sib : affected() )
        sib->paint(painter, t, mode, const_cast<glaxnimate::model::PathModifier*>(this));
}


