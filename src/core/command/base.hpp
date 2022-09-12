#pragma once

#include <QUndoCommand>

namespace glaxnimate::command {


enum class Id {
    SetPropertyValue,
    SetMultipleProperties,
    SetKeyframe,
    SetMultipleAnimated,
    SetPositionBezier,

    // For additional commands, use values increadising from here
    CustomCommand,
};

template<Id id_enum, class Derived>
class MergeableCommand : public QUndoCommand
{
public:
    int id() const final
    {
        return int(id_enum);
    }

    bool mergeWith ( const QUndoCommand * other ) final
    {
        if ( commit )
            return false;
        auto oth = static_cast<const Derived*>(other);
        if ( static_cast<Derived*>(this)->merge_with(*oth) )
        {
            commit = oth->commit;
            return true;
        }
        return false;
    }

protected:
    using Parent = MergeableCommand;

    MergeableCommand(const QString& name, bool commit = true)
        : QUndoCommand(name), commit(commit)
    {}

    bool commit;
};

} // namespace glaxnimate::command
