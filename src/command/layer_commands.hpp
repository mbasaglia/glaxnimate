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
          position(position)
    {}


    void undo() override
    {
        layer = parent->remove_layer(index);
    }

    void redo() override
    {
        index = parent->add_layer(std::move(layer), position);
    }

private:
    model::Composition* parent;
    std::unique_ptr<model::Layer> layer;
    int index = -1;
    int position;
};

} // namespace command

