#include "create_items.hpp"

#include "document_node_graphics_item.hpp"
#include "main_composition_item.hpp"

graphics::DocumentNodeGraphicsItem * graphics::docnode_make_graphics_item(model::DocumentNode* node)
{
    if ( auto mcom = qobject_cast<model::MainComposition*>(node) )
        return new MainCompositionItem(mcom);

    auto item = new DocumentNodeGraphicsItem(node);

    if ( auto layer = qobject_cast<model::Layer*>(node) )
    {
        QObject::connect(layer, &model::Layer::transform_matrix_changed, item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix);
    }

    return item;
}

std::vector<std::unique_ptr<QGraphicsItem> > graphics::docnode_make_graphics_editor(model::DocumentNode* node)
{
    std::vector<std::unique_ptr<QGraphicsItem>> v;


    if ( auto layer = qobject_cast<model::Layer*>(node) )
    {
        auto p = std::make_unique<graphics::TransformGraphicsItem>(layer->transform.get(), layer, nullptr);
        QObject::connect(layer, &model::Layer::transform_matrix_changed, p.get(), &graphics::TransformGraphicsItem::set_transform_matrix);
        p->set_transform_matrix(layer->transform_matrix());
        v.push_back(std::move(p));
    }
    else if ( auto mcomp = qobject_cast<model::MainComposition*>(node) )
    {
        v.push_back(std::make_unique<graphics::MainCompositionTransformItem>(mcomp));
    }


    return v;;
}


