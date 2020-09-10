#pragma once
#include <QUndoCommand>

#include "model/composition.hpp"

namespace command {

class AddLayer : public QUndoCommand
{
public:
    AddLayer(model::Composition* parent, std::unique_ptr<model::Layer> layer, int position)
        : QUndoCommand(QObject::tr("Create %1").arg(layer->docnode_name())),
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
        : QUndoCommand(QObject::tr("Remove %1").arg(layer->docnode_name())),
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


} // namespace command

