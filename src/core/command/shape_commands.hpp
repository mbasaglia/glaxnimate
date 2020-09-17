#pragma once
#include <QUndoCommand>

#include "model/shapes/shape.hpp"

namespace model { class Group; }

namespace command {

class AddShape : public QUndoCommand
{
public:
    AddShape(
        model::ShapeListProperty* shape_parent,
        std::unique_ptr<model::ShapeElement> shape,
        int position,
        QUndoCommand* parent = nullptr
    )
        : QUndoCommand(QObject::tr("Create %1").arg(shape->docnode_name()), parent),
          shape_parent(shape_parent),
          shape(std::move(shape)),
          position(position)
    {}

    void undo() override
    {
        shape = shape_parent->remove(position);
    }

    void redo() override
    {
        shape_parent->insert(std::move(shape), position);
    }

private:
    model::ShapeListProperty* shape_parent;
    std::unique_ptr<model::ShapeElement> shape;
    int position;
};


class RemoveShape : public QUndoCommand
{
public:
    RemoveShape(model::ShapeElement* shape, QUndoCommand* parent = nullptr)
        : QUndoCommand(QObject::tr("Remove %1").arg(shape->docnode_name()), parent),
          shape_parent(shape->owner()),
          position(shape->position())
    {}


    void undo() override
    {
        shape_parent->insert(std::move(shape), position);
    }

    void redo() override
    {
        shape = shape_parent->remove(position);
    }

private:
    model::ShapeListProperty* shape_parent;
    std::unique_ptr<model::ShapeElement> shape;
    int position;
};


class MoveShape : public QUndoCommand
{
public:
    MoveShape(
        model::ShapeElement* shape,
        model::ShapeListProperty* parent_after,
        int position_after,
        QUndoCommand* parent = nullptr
    )
        : QUndoCommand(QObject::tr("Move Shape"), parent),
          parent_before(shape->owner()),
          position_before(parent_before->index_of(shape, -1)),
          parent_after(parent_after),
          position_after(position_after)
    {}

    void undo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto shape = parent_after->remove(position_after) )
            parent_before->insert(std::move(shape), position_before);
    }

    void redo() override
    {
        if ( parent_before == parent_after )
            parent_before->move(position_before, position_after);
        else if ( auto shape = parent_before->remove(position_before) )
            parent_after->insert(std::move(shape), position_after);
    }

private:
    model::ShapeListProperty* parent_before;
    int position_before;
    model::ShapeListProperty* parent_after;
    int position_after;
};


class GroupShapes : public QUndoCommand
{
public:
    struct Data
    {
        std::vector<model::ShapeElement*> elements;
        model::ShapeListProperty* parent = nullptr;
    };

    GroupShapes(const Data& data);

    static Data collect_shapes(const std::vector<model::DocumentNode *>& selection);

    void undo() override;
    void redo() override;


private:
    model::Group* group = nullptr;
    bool did = true;
};

} // namespace command


