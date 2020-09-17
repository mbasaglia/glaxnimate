#include "shape_commands.hpp"
#include "model/shapes/group.hpp"
#include "model/layers/shape_layer.hpp"
#include "model/document.hpp"


namespace {
model::DocumentNode* shape_parent(model::ShapeElement* shape)
{
    return static_cast<model::ShapeElement*>(shape->owner()->object());
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

    PathToLayer(model::DocumentNode* node)
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
        if ( steps.empty() )
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

        return nullptr;
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
        collected = shape_parent(selection[i]);
        data.parent = collected.lowest();
    }

    for ( i++; i < int(selection.size()) && data.parent; i++ )
    {
        data.parent = collected.combine(shape_parent(selection[i]));
        if ( !data.parent )
            return {};
    }

    data.elements.reserve(selection.size());
    for ( auto n : selection )
        data.elements.push_back(static_cast<model::ShapeElement*>(n));
    return data;
}

command::GroupShapes::GroupShapes(const command::GroupShapes::Data& data)
    : QUndoCommand(QObject::tr("Group Shapes"))
{
    if ( data.parent )
    {
        children.reserve(data.elements.size() + 1);
        std::unique_ptr<model::Group> grp = std::make_unique<model::Group>(data.parent->object()->document());
        group = grp.get();
        data.parent->object()->document()->set_best_name(group);
        children.push_back(std::make_unique<AddShape>(data.parent, std::move(grp), data.parent->size()));
        for ( int i = 0; i < int(data.elements.size()); i++ )
            children.push_back(std::make_unique<MoveShape>(data.elements[i], &group->shapes, i));
    }
}

void command::GroupShapes::redo()
{
    for ( const auto& ch : children )
        ch->redo();
}

void command::GroupShapes::undo()
{
    for ( int i = int(children.size()) - 1; i >= 0; i-- )
        children[i]->undo();
}




