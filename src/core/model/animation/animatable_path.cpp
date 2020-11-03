#include "animatable_path.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/animation_commands.hpp"


void model::detail::AnimatedPropertyBezier::set_closed(bool closed)
{
    value_.set_closed(closed);
    for ( auto& keyframe : keyframes_ )
        keyframe->value_.set_closed(closed);
    value_changed();
    emitter(object(), value_);
}


void model::detail::AnimatedPropertyBezier::split_segment(int index, qreal factor)
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

void model::detail::AnimatedPropertyBezier::remove_point(int index)
{
    remove_points({index});
}

static math::bezier::Bezier remove_points(const math::bezier::Bezier& bez, const std::set<int>& indices)
{
    math::bezier::Bezier new_bez;
    new_bez.set_closed(bez.closed());

    for ( int i = 0; i < bez.size(); i++ )
        if ( !indices.count(i) )
            new_bez.push_back(bez[i]);

    return new_bez;
}

void model::detail::AnimatedPropertyBezier::remove_points(const std::set<int>& indices)
{
    command::UndoMacroGuard guard(tr("Remove Nodes"), object()->document());

    QVariant before = QVariant::fromValue(value_);
    auto bez = value_;

    bool set = true;
    for ( const auto& kf : keyframes_ )
    {
        auto bez = ::remove_points(kf->value_, indices);
        if ( !mismatched_ && kf->time() == time() )
            set = false;
        object()->push_command(new command::SetKeyframe(this, kf->time(), QVariant::fromValue(bez), true));
    }

    if ( set )
    {
        bez = ::remove_points(bez, indices);
        object()->push_command(new command::SetMultipleAnimated(this, QVariant::fromValue(bez), true));
    }
}
