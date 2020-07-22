#pragma once

#include <QUndoCommand>

namespace command {


enum class Id {
    SetPropertyValue,
    SetMultipleProperties
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
        return static_cast<Derived*>(this)->merge_with(*static_cast<const Derived*>(other));
    }

protected:
    using Parent = MergeableCommand;

    MergeableCommand(const QString& name, bool commit = true)
        : QUndoCommand(name), commit(commit)
    {}

    bool commit;
};

} // namespace command
