#include "create_items.hpp"

#include "document_node_graphics_item.hpp"
#include "main_composition_item.hpp"
#include "model/shapes/shapes.hpp"
#include "position_item.hpp"
#include "sizepos_item.hpp"
#include "bezier_item.hpp"
#include "rect_rounder.hpp"
#include "graphics_editor.hpp"
#include "star_radius_item.hpp"

graphics::GraphicsItemFactory::GraphicsItemFactory()
{
    register_builder<model::MainComposition>(
        [](model::MainComposition* mcomp){
            return new MainCompositionItem(mcomp);
        },
        [](model::MainComposition* mcomp){
            auto v = std::make_unique<GraphicsEditor>(mcomp);
            v->add_child<graphics::MainCompositionTransformItem>(mcomp);
            return v;
        }
    );
    register_builder<model::Layer>(
        [](model::Layer* layer){
            auto item = new DocumentNodeGraphicsItem(layer);
            item->set_transform_matrix(layer->transform_matrix(layer->time()));
            QObject::connect(layer, &model::Layer::transform_matrix_changed, item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix);
            return item;
        },
        [](model::Layer* layer){
            auto v = std::make_unique<GraphicsEditor>(layer);
            v->add_child<graphics::TransformGraphicsItem>(layer->transform.get(), layer, nullptr);
            return v;
        }
    );

    auto make_item_for_modifier = [](model::ShapeOperator* shape){
        auto item = GraphicsItemFactory::make_graphics_item_default(shape);
        QObject::connect(shape, &model::ShapeOperator::shape_changed,
                         item, &DocumentNodeGraphicsItem::shape_changed);
        return item;
    };

    register_builder<model::Rect>(
        &GraphicsItemFactory::make_graphics_item_default,
        [](model::Rect* rect){
            auto v = std::make_unique<GraphicsEditor>(rect);
            v->add_child<graphics::PositionItem>(&rect->position);
            v->add_child<graphics::RectRounder>(rect);
            v->add_child<graphics::SizePosItem>(&rect->size, &rect->position);
            return v;
        }
    );
    register_builder<model::Ellipse>(
        &GraphicsItemFactory::make_graphics_item_default,
        [](model::Ellipse* rect){
            auto v = std::make_unique<GraphicsEditor>(rect);
            v->add_child<graphics::PositionItem>(&rect->position);
            v->add_child<graphics::SizePosItem>(&rect->size, &rect->position);
            return v;
        }
    );
    register_builder<model::PolyStar>(
        &GraphicsItemFactory::make_graphics_item_default,
        [](model::PolyStar* star){
            auto v = std::make_unique<GraphicsEditor>(star);
            v->add_child<graphics::PositionItem>(&star->position);
            v->add_child<graphics::StarRadiusItem>(star);
            return v;
        }
    );
    register_builder<model::Path>(
        &GraphicsItemFactory::make_graphics_item_default,
        [](model::Path* shape){
            auto v = std::make_unique<GraphicsEditor>(shape);
            v->add_child<graphics::BezierItem>(shape);
            return v;
        }
    );
    register_builder<model::Group>(
        [](model::Group* layer){
            auto item = new DocumentNodeGraphicsItem(layer);
            item->set_transform_matrix(layer->local_transform_matrix(layer->time()));
            QObject::connect(layer, &model::Group::local_transform_matrix_changed,
                             item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix);
            return item;
        },
        [](model::Group* layer){
            auto v = std::make_unique<GraphicsEditor>(layer);
            v->add_child<graphics::TransformGraphicsItem>(layer->transform.get(), layer, nullptr);
            return v;
        }
    );
    register_builder<model::ShapeOperator>(
        make_item_for_modifier,
        &GraphicsItemFactory::make_graphics_editor_default
    );
}

graphics::DocumentNodeGraphicsItem * graphics::GraphicsItemFactory::make_graphics_item(model::DocumentNode* node) const
{
    if ( auto builder = builder_for(node) )
        return builder->make_graphics_item(node);
    return make_graphics_item_default(node);
}

std::unique_ptr<graphics::GraphicsEditor> graphics::GraphicsItemFactory::make_graphics_editor(model::DocumentNode* node) const
{
    if ( auto builder = builder_for(node) )
        return builder->make_graphics_editor(node);
    return make_graphics_editor_default(node);
}

graphics::DocumentNodeGraphicsItem * graphics::GraphicsItemFactory::make_graphics_item_default(model::DocumentNode* node)
{
    return new DocumentNodeGraphicsItem(node);
}

std::unique_ptr<graphics::GraphicsEditor> graphics::GraphicsItemFactory::make_graphics_editor_default(model::DocumentNode*)
{
    return {};
}

graphics::GraphicsItemFactory::AbstractBuilder * graphics::GraphicsItemFactory::builder_for(model::DocumentNode* node) const
{
    const QMetaObject* mo = node->metaObject();
     auto it = builders.find(mo);
     while ( mo && it == builders.end() )
     {
         mo = mo->superClass();
         it = builders.find(mo);
     }

     if ( mo )
         return it->second;

     return nullptr;
}
