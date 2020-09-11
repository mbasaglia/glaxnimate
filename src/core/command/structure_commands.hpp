#pragma once

#include <memory>
#include <QUndoCommand>

#include "model/document_node.hpp"
#include "model/shapes/shape.hpp"
#include "model/layers/layer.hpp"

namespace command {

class DeferredCommandBase : public QUndoCommand
{
public:
    ~DeferredCommandBase();

    void undo() override;
    void redo() override;

    bool has_action() const;

protected:
    DeferredCommandBase(const QString& name, std::unique_ptr<QUndoCommand> d = {})
    : QUndoCommand(name), d(std::move(d))
    {}

    std::unique_ptr<QUndoCommand> d;
};

class DuplicateCommand : public DeferredCommandBase
{
public:
    DuplicateCommand(model::DocumentNode* node);
    DuplicateCommand(model::Layer* node);
    DuplicateCommand(model::ShapeElement* node);

private:
    static QString name(model::DocumentNode* node);

};

class DeleteCommand : public DeferredCommandBase
{
public:
    DeleteCommand(model::DocumentNode* node);
    DeleteCommand(model::Layer* node);
    DeleteCommand(model::ShapeElement* node);

private:
    static QString name(model::DocumentNode* node);
};


} // namespace command
