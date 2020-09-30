#pragma once

#include <QUndoCommand>

#include "model/property/object_list_property.hpp"

namespace command {

template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class AddObject : public QUndoCommand
{
public:
    AddObject(
        PropT* object_parent,
        std::unique_ptr<ItemT> object,
        int position,
        QUndoCommand* parent = nullptr,
        const QString& name = {}
    )
        : QUndoCommand(name.isEmpty() ? QObject::tr("Create %1").arg(object->object_name()) : name, parent),
          object_parent(object_parent),
          object(std::move(object)),
          position(position)
    {}

    void undo() override
    {
        object = object_parent->remove(position);
    }

    void redo() override
    {
        object_parent->insert(std::move(object), position);
    }

private:
    PropT* object_parent;
    std::unique_ptr<ItemT> object;
    int position;
};


template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class RemoveObject : public QUndoCommand
{
public:
    RemoveObject(ItemT* object, PropT* object_parent, QUndoCommand* parent = nullptr)
        : QUndoCommand(QObject::tr("Remove %1").arg(object->object_name()), parent),
          object_parent(object_parent),
          position(object_parent->index_of(object, -1))
    {}

    RemoveObject(int index, PropT* object_parent, QUndoCommand* parent = nullptr)
        : QUndoCommand(QObject::tr("Remove %1").arg((*object_parent)[index]->object_name()), parent),
          object_parent(object_parent),
          position(index)
    {}


    void undo() override
    {
        object_parent->insert(std::move(object), position);
    }

    void redo() override
    {
        object = object_parent->remove(position);
    }

private:
    PropT* object_parent;
    std::unique_ptr<ItemT> object;
    int position;
};


template<class ItemT, class PropT = model::ObjectListProperty<ItemT>>
class MoveObject : public QUndoCommand
{
public:
    MoveObject(
        ItemT* object,
        PropT* parent_before,
        PropT* parent_after,
        int position_after,
        QUndoCommand* parent = nullptr
    )
        : QUndoCommand(QObject::tr("Move Object"), parent),
          parent_before(parent_before),
          position_before(parent_before->index_of(object, -1)),
          parent_after(parent_after),
          position_after(position_after)
    {}

    void undo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto object = parent_after->remove(position_after) )
            parent_before->insert(std::move(object), position_before);
    }

    void redo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto object = parent_before->remove(position_before) )
            parent_after->insert(std::move(object), position_after);
    }

private:
    PropT* parent_before;
    int position_before;
    PropT* parent_after;
    int position_after;
};


} // namespace command
