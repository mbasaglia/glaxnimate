/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QUndoCommand>

#include "model/shapes/shape.hpp"
#include "object_list_commands.hpp"

namespace glaxnimate::model { class Group; }

namespace glaxnimate::command {

using AddShape = AddObject<model::ShapeElement, model::ShapeListProperty>;
using RemoveShape = RemoveObject<model::ShapeElement, model::ShapeListProperty>;
using MoveShape = MoveObject<model::ShapeElement, model::ShapeListProperty>;

namespace detail {

class RedoInCtor : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;

protected:
    using QUndoCommand::QUndoCommand;

private:
    bool did = true;
};

} // namespace detail

class GroupShapes : public detail::RedoInCtor
{
public:
    struct Data
    {
        std::vector<model::ShapeElement*> elements;
        model::ShapeListProperty* parent = nullptr;
    };

    GroupShapes(const Data& data);

    static Data collect_shapes(const std::vector<model::VisualNode *>& selection);


private:
    model::Group* group = nullptr;
};

class UngroupShapes : public detail::RedoInCtor
{
public:
    UngroupShapes(model::Group* group);

};

AddShape* duplicate_shape(model::ShapeElement* shape);

} // namespace glaxnimate::command
