#pragma once
#include <QUndoCommand>

#include "model/composition.hpp"

namespace command {

class AddLayer : public QUndoCommand
{
public:
    AddLayer(model::Composition* parent, std::unique_ptr<model::Layer> layer, int position)
        : QUndoCommand(QObject::tr("Create %1").arg(layer->object_name())),
          parent(parent),
          layer(std::move(layer)),
          uuid(this->layer->uuid.get()),
          position(position)
    {}


    void undo() override
    {
        layer = parent->remove_layer(uuid);
    }

    void redo() override
    {
        parent->add_layer(std::move(layer), position);
    }

private:
    model::Composition* parent;
    std::unique_ptr<model::Layer> layer;
    QUuid uuid;
    int position;
};


class RemoveLayer : public QUndoCommand
{
public:
    RemoveLayer(model::Layer* layer)
        : QUndoCommand(QObject::tr("Remove %1").arg(layer->object_name())),
          parent(layer->composition()),
          uuid(layer->uuid.get()),
          position(parent->layer_position(layer, -1))
    {}


    void undo() override
    {
        parent->add_layer(std::move(layer), position);
    }

    void redo() override
    {
        layer = parent->remove_layer(uuid);
    }

private:
    model::Composition* parent;
    std::unique_ptr<model::Layer> layer;
    QUuid uuid;
    int position;
};

class MoveLayer : public QUndoCommand
{
public:
    MoveLayer(model::Layer* layer, model::Composition* parent_after, int position_after)
        : QUndoCommand(QObject::tr("Move Layer")),
          parent_before(layer->composition()),
          position_before(parent_before->layer_position(layer, -1)),
          parent_after(parent_after),
          position_after(position_after)
    {}

    void undo() override
    {
        if ( parent_before == parent_after )
            parent_before->layers.move(position_before, position_after);
        else if ( auto layer = parent_after->layers.remove(position_after) )
            parent_before->add_layer(std::move(layer), position_before);
    }

    void redo() override
    {
        if ( parent_before == parent_after )
            parent_before->layers.move(position_before, position_after);
        else if ( auto layer = parent_before->layers.remove(position_before) )
            parent_after->add_layer(std::move(layer), position_after);
    }

private:
    model::Composition* parent_before;
    int position_before;
    model::Composition* parent_after;
    int position_after;
};


} // namespace command

