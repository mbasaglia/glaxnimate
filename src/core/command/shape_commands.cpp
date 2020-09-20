#include "shape_commands.hpp"
#include "model/shapes/group.hpp"
#include "model/layers/shape_layer.hpp"
#include "model/document.hpp"


namespace {
model::DocumentNode* shape_parent(model::ShapeElement* shape)
{
    return static_cast<model::DocumentNode*>(shape->owner()->object());
}

model::DocumentNode* shape_parent(model::DocumentNode* shape)
{
    if ( auto se = qobject_cast<model::ShapeElement*>(shape) )
        return shape_parent(se);
    return nullptr;
}

struct PathToLayer
{
    PathToLayer() = default;

    explicit PathToLayer(model::DocumentNode* node)
    {
        layer = nullptr;
        while ( node && !layer )
        {
            layer = qobject_cast<model::ShapeLayer*>(node);
            if ( layer )
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
    model::ShapeLayer* layer = nullptr;

    model::ShapeListProperty* lowest() const
    {
        if ( !steps.empty() )
            return &steps.back()->shapes;
        return &layer->shapes;
    }

    model::ShapeListProperty* combine(const PathToLayer& other)
    {
        if ( other.layer != layer )
            return nullptr;

        int i = 0;
        for ( int e = std::min(steps.size(), other.steps.size()); i < e; i++ )
            if ( steps[i] != other.steps[i] )
                break;

        if ( i < int(steps.size()) )
        {
            steps.erase(steps.begin()+i, steps.end());
            return lowest();
        }

        return &layer->shapes;
    }
};

} // namespace

command::GroupShapes::Data command::GroupShapes::collect_shapes(const std::vector<model::DocumentNode *>& selection)
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
            (new MoveShape(data.elements[i], &group->shapes, i, this))->redo();
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
    (new RemoveShape(group, this))->redo();
    for ( int i = 0, e = group->shapes.size(); i < e; i++ )
    {
        (new MoveShape(&group->shapes[0], group->owner(), pos+i, this))->redo();
    }
}

