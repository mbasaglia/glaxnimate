#include "shape_commands.hpp"
#include "model/shapes/group.hpp"
#include "model/composition.hpp"
#include "model/document.hpp"


namespace {

/**
 * \returns The parent node for \p shape
 */
model::VisualNode* shape_parent(model::ShapeElement* shape)
{
    return static_cast<model::VisualNode*>(shape->owner()->object());
}

/**
 * \returns The parent node for \p shape
 */
model::VisualNode* shape_parent(model::VisualNode* shape)
{
    if ( auto se = qobject_cast<model::ShapeElement*>(shape) )
        return shape_parent(se);
    return nullptr;
}

/**
 * \brief Represents a sequence of nested nodes to reach
 */
struct PathToLayer
{
    PathToLayer() = default;

    explicit PathToLayer(model::VisualNode* node)
    {
        composition = nullptr;
        while ( node && !composition )
        {
            composition = qobject_cast<model::Composition*>(node);
            if ( composition )
                break;

            if ( auto group = qobject_cast<model::Group*>(node) )
            {
                steps.push_back(group);
                node = shape_parent(group);
            }
            else
            {
                return;
            }
        }
    }

    std::vector<model::Group*> steps;
    model::Composition* composition = nullptr;

    model::ShapeListProperty* lowest() const
    {
        if ( !steps.empty() )
            return &steps.front()->shapes;
        return &composition->shapes;
    }

    model::ShapeListProperty* combine(const PathToLayer& other)
    {
        if ( other.composition != composition )
            return nullptr;

        int i = 0;
        for ( int e = std::min(steps.size(), other.steps.size()); i < e; i++ )
            if ( steps[i] != other.steps[i] )
                break;

        if ( i < int(steps.size()) )
            steps.erase(steps.begin()+i, steps.end());

        return lowest();
    }
};

} // namespace

command::GroupShapes::Data command::GroupShapes::collect_shapes(const std::vector<model::VisualNode *>& selection)
{
    if ( selection.empty() )
        return {};

    Data data;
    PathToLayer collected;

    int i = 0;
    for ( ; i < int(selection.size()) && !data.parent; i++ )
    {
        collected = PathToLayer(shape_parent(selection[i]));
        data.parent = collected.lowest();
    }

    for ( ; i < int(selection.size()) && data.parent; i++ )
    {
        data.parent = collected.combine(PathToLayer(shape_parent(selection[i])));
        if ( !data.parent )
            return {};
    }

    data.elements.reserve(selection.size());
    for ( auto n : selection )
        data.elements.push_back(static_cast<model::ShapeElement*>(n));
    return data;
}

command::GroupShapes::GroupShapes(const command::GroupShapes::Data& data)
    : detail::RedoInCtor(QObject::tr("Group Shapes"))
{
    if ( data.parent )
    {
        std::unique_ptr<model::Group> grp = std::make_unique<model::Group>(data.parent->object()->document());
        group = grp.get();
        data.parent->object()->document()->set_best_name(group);
        (new AddShape(data.parent, std::move(grp), data.parent->size(), this))->redo();

        for ( int i = 0; i < int(data.elements.size()); i++ )
        {
            (new MoveShape(data.elements[i], data.elements[i]->owner(), &group->shapes, i, this))->redo();
        }
    }
}

void command::detail::RedoInCtor::redo()
{
    if ( !did )
    {
        QUndoCommand::redo();
        did = true;
    }
}

void command::detail::RedoInCtor::undo()
{
    QUndoCommand::undo();
    did = false;
}


command::UngroupShapes::UngroupShapes(model::Group* group)
    : detail::RedoInCtor(QObject::tr("Ungroup Shapes"))
{
    int pos = group->owner()->index_of(group);
    (new RemoveShape(group, group->owner(), this))->redo();
    for ( int i = 0, e = group->shapes.size(); i < e; i++ )
    {
        (new MoveShape(group->shapes[0], group->shapes[0]->owner(), group->owner(), pos+i, this))->redo();
    }
}


command::AddShape * command::duplicate_shape ( model::ShapeElement* shape )
{
    std::unique_ptr<model::ShapeElement> new_shape (
        static_cast<model::ShapeElement*>(shape->clone().release())
    );
    new_shape->refresh_uuid();
    new_shape->recursive_rename();
    new_shape->set_time(shape->docnode_parent()->time());

    return new command::AddShape(
        shape->owner(),
        std::move(new_shape),
        shape->owner()->index_of(shape)+1,
        nullptr,
        QObject::tr("Duplicate %1").arg(shape->object_name())
    );
}

