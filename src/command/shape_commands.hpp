#pragma once
#include <QUndoCommand>

#include "model/shapes/shape.hpp"

namespace command {

class AddShape : public QUndoCommand
{
public:
    AddShape(model::ShapeListProperty* parent, std::unique_ptr<model::ShapeElement> shape, int position)
        : QUndoCommand(QObject::tr("Create %1").arg(shape->docnode_name())),
          parent(parent),
          shape(std::move(shape)),
          position(position)
    {}


    void undo() override
    {
        shape = parent->remove(position);
    }

    void redo() override
    {
        parent->insert(std::move(shape), position);
    }

private:
    model::ShapeListProperty* parent;
    std::unique_ptr<model::ShapeElement> shape;
    int position;
};


class RemoveShape : public QUndoCommand
{
public:
    RemoveShape(model::ShapeListProperty* parent, model::ShapeElement* shape)
        : QUndoCommand(QObject::tr("Remove %1").arg(shape->docnode_name())),
          parent(parent),
          shape(shape),
          position(shape->position())
    {}


    void undo() override
    {
        shape = parent->remove(position);
    }

    void redo() override
    {
        parent->insert(std::move(shape), position);
    }

private:
    model::ShapeListProperty* parent;
    std::unique_ptr<model::ShapeElement> shape;
    int position;
};


} // namespace command


