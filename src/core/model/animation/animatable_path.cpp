#include "animatable_path.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/animation_commands.hpp"


void model::AnimatedProperty<math::bezier::Bezier>::split_segment(int index, qreal factor)
{
    command::UndoMacroGuard guard(tr("Split Segment"), object()->document());
    bool set = true;
    for ( const auto& kf : keyframes_ )
    {
        auto bez = kf->get();
        bez.split_segment(index, factor);
        if ( kf->time() == time() )
        {
            set = false;
            value_ = bez;
        }
        object()->push_command(new command::SetKeyframe(this, kf->time(), QVariant::fromValue(bez), true));
    }

    if ( set )
    {
        QVariant before = QVariant::fromValue(value_);
        value_.split_segment(index, factor);
        QVariant after = QVariant::fromValue(value_);
        object()->push_command(new command::SetMultipleAnimated("", {this}, {before}, {after}, true));
    }

    value_changed();
}

