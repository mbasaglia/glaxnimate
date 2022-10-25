#include "animatable_path.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/animation_commands.hpp"

using namespace glaxnimate;

void glaxnimate::model::detail::AnimatedPropertyBezier::set_closed(bool closed)
{
    value_.set_closed(closed);
    for ( auto& keyframe : keyframes_ )
        keyframe->value_.set_closed(closed);
    value_changed();
    emitter(object(), value_);
}


void glaxnimate::model::detail::AnimatedPropertyBezier::split_segment(int index, qreal factor)
{
    command::UndoMacroGuard guard(tr("Split Segment"), object()->document());

    QVariant before = QVariant::fromValue(value_);
    auto bez = value_;

    bool set = true;
    for ( const auto& kf : keyframes_ )
    {
        auto bez = kf->get();
        bez.split_segment(index, factor);
        if ( !mismatched_ && kf->time() == time() )
            set = false;
        object()->push_command(new command::SetKeyframe(this, kf->time(), QVariant::fromValue(bez), true));
    }

    if ( set )
    {
        bez.split_segment(index, factor);
        QVariant after = QVariant::fromValue(bez);
        object()->push_command(new command::SetMultipleAnimated("", {this}, {before}, {after}, true));
    }
}

void glaxnimate::model::detail::AnimatedPropertyBezier::remove_point(int index)
{
    remove_points({index});
}

void glaxnimate::model::detail::AnimatedPropertyBezier::remove_points(const std::set<int>& indices)
{
    command::UndoMacroGuard guard(tr("Remove Nodes"), object()->document());

    QVariant before = QVariant::fromValue(value_);
    auto bez = value_;

    bool set = true;
    for ( const auto& kf : keyframes_ )
    {
        auto bez = kf->value_.removed_points(indices);
        if ( !mismatched_ && kf->time() == time() )
            set = false;
        object()->push_command(new command::SetKeyframe(this, kf->time(), QVariant::fromValue(bez), true));
    }

    if ( set )
    {
        bez = bez.removed_points(indices);
        object()->push_command(new command::SetMultipleAnimated(this, QVariant::fromValue(bez), true));
    }
}

static QVariant extend_impl(math::bezier::Bezier subject, const math::bezier::Bezier& target, bool at_end)
{
    if ( target.closed() )
    {
        subject.set_closed(true);

        if ( !subject.empty() )
        {
            if ( at_end )
                subject[0].type = math::bezier::Corner;
            else
                subject.back().type = math::bezier::Corner;

            if ( !target.empty() )
            {
                subject[0].tan_in = target[0].tan_in;
                subject.back().tan_out = target.back().tan_out;
            }
        }
    }

    if ( subject.size() < target.size() )
    {
        if ( at_end )
        {
            if ( !subject.empty() )
            {
                subject.back().type = math::bezier::Corner;
                subject.back().tan_out = target.back().tan_out;
            }

            subject.points().insert(
                subject.points().end(),
                target.points().begin() + subject.size(),
                target.points().end()
            );
        }
        else
        {
            if ( !subject.empty() )
            {
                subject[0].type = math::bezier::Corner;
                subject[0].tan_in = target[0].tan_in;
            }

            subject.points().insert(
                subject.points().begin(),
                target.points().begin(),
                target.points().begin() + target.size() - subject.size()
            );
        }
    }

    return QVariant::fromValue(subject);
}

void glaxnimate::model::detail::AnimatedPropertyBezier::extend(const math::bezier::Bezier& target, bool at_end)
{
    command::UndoMacroGuard guard(tr("Extend Shape"), object()->document());

    auto bez = value_;

    bool set = true;

    for ( auto& kf : keyframes_ )
    {
        if ( !mismatched_ && kf->time() == time() )
            set = false;
        object()->push_command(
            new command::SetKeyframe(this, kf->time(), extend_impl(kf->value_, target, at_end), true)
        );
    }

    if ( set )
    {
        QVariant before = QVariant::fromValue(bez);
        QVariant after = extend_impl(bez, target, at_end);
        object()->push_command(new command::SetMultipleAnimated("", {this}, {before}, {after}, true));
    }
}
