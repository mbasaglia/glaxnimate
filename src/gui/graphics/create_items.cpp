#include "create_items.hpp"

#include "document_node_graphics_item.hpp"
#include "main_composition_item.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/text.hpp"

#include "position_item.hpp"
#include "sizepos_item.hpp"
#include "bezier_item.hpp"
#include "rect_rounder.hpp"
#include "graphics_editor.hpp"
#include "star_radius_item.hpp"
#include "shape_graphics_item.hpp"
#include "gradient_editor.hpp"
#include "text_attributes_editor.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

static graphics::DocumentNodeGraphicsItem * make_graphics_item_shape(model::ShapeElement* node)
{
    return new graphics::ShapeGraphicsItem(node);
}

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

    auto make_item_for_modifier = [](model::ShapeOperator* shape){
        auto item = GraphicsItemFactory::make_graphics_item_default(shape);
        QObject::connect(shape, &model::ShapeOperator::shape_changed,
                         item, &DocumentNodeGraphicsItem::shape_changed);
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        return item;
    };

    register_builder<model::Rect>(
        &make_graphics_item_shape,
        [](model::Rect* rect){
            auto v = std::make_unique<GraphicsEditor>(rect);
            v->add_child<graphics::PositionItem>(&rect->position);
            if ( rect->position.keyframe_count() >= 2 )
                v->add_child<graphics::BezierItem>(&rect->position);
            v->add_child<graphics::RectRounder>(rect);
            v->add_child<graphics::SizePosItem>(&rect->size, &rect->position);
            return v;
        }
    );
    register_builder<model::Ellipse>(
        &make_graphics_item_shape,
        [](model::Ellipse* rect){
            auto v = std::make_unique<GraphicsEditor>(rect);
            v->add_child<graphics::PositionItem>(&rect->position);
            if ( rect->position.keyframe_count() >= 2 )
                v->add_child<graphics::BezierItem>(&rect->position);
            v->add_child<graphics::SizePosItem>(&rect->size, &rect->position);
            return v;
        }
    );
    register_builder<model::PolyStar>(
        &make_graphics_item_shape,
        [](model::PolyStar* star){
            auto v = std::make_unique<GraphicsEditor>(star);
            if ( star->position.keyframe_count() >= 2 )
                v->add_child<graphics::BezierItem>(&star->position);
            v->add_child<graphics::PositionItem>(&star->position);
            v->add_child<graphics::StarRadiusItem>(star);
            return v;
        }
    );
    register_builder<model::Path>(
        &make_graphics_item_shape,
        [](model::Path* shape){
            auto v = std::make_unique<GraphicsEditor>(shape);
            auto editor = v->add_child<graphics::BezierItem>(&shape->shape);
            QObject::connect(shape, &model::Path::shape_changed, editor, &graphics::BezierItem::update_bezier);
            return v;
        }
    );
    register_builder<model::Group>(
        [](model::Group* group){
            DocumentNodeGraphicsItem* item;
            if ( auto layer = qobject_cast<model::Layer*>(group) )
            {
                item = new DocumentNodeGraphicsItem(layer);
                item->set_visible_permitted(layer->animation->time_visible());
                QObject::connect(layer->animation.get(), &model::AnimationContainer::time_visible_changed,
                                 item, &graphics::DocumentNodeGraphicsItem::set_visible_permitted);
                item->set_selection_mode(DocumentNodeGraphicsItem::None);
            }
            else
            {
                item = new graphics::ShapeGraphicsItem(group);
                item->set_selection_mode(DocumentNodeGraphicsItem::Group);
            }
            item->set_transform_matrix(group->group_transform_matrix(group->time()));
            QObject::connect(group, &model::Group::group_transform_matrix_changed,
                             item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix);
            QObject::connect(group, &model::Group::opacity_changed, item, &graphics::DocumentNodeGraphicsItem::set_opacity);
            item->set_opacity(group->opacity.get());

            return item;
        },
        [](model::Group* layer){
            auto v = std::make_unique<GraphicsEditor>(layer);
            v->add_child<graphics::TransformGraphicsItem>(layer->transform.get(), layer, nullptr);
            if ( layer->transform.get()->position.keyframe_count() >= 2 )
                v->add_child<graphics::BezierItem>(&layer->transform.get()->position);
            return v;
        }
    );
    register_builder<model::Image>(
        [](model::Image* shape){
            auto item = new DocumentNodeGraphicsItem(shape);
            item->set_transform_matrix(shape->local_transform_matrix(shape->time()));
            QObject::connect(shape, &model::Image::local_transform_matrix_changed,
                             item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix);
            return item;
        },
        [](model::Image* shape){
            auto v = std::make_unique<GraphicsEditor>(shape);
            v->add_child<graphics::TransformGraphicsItem>(shape->transform.get(), shape, nullptr);
            return v;
        }
    );
    register_builder<model::Styler>(
        make_item_for_modifier,
        [](model::Styler* styler){
            auto v = std::make_unique<GraphicsEditor>(styler);
            v->add_child<graphics::GradientEditor>(styler);
            return v;
        }
    );
    register_builder<model::Precomposition>(
        [](model::Precomposition* comp){
            return new CompositionItem(comp);
        },
        [](model::Precomposition* comp){
            auto v = std::make_unique<GraphicsEditor>(comp);
            return v;
        }
    );
    register_builder<model::PreCompLayer>(
        [](model::PreCompLayer* shape){
            auto item = new DocumentNodeGraphicsItem(shape);
            item->set_transform_matrix(shape->local_transform_matrix(shape->time()));
            QObject::connect(
                shape, &model::Image::local_transform_matrix_changed,
                item, &graphics::DocumentNodeGraphicsItem::set_transform_matrix
            );
            QObject::connect(
                shape->document(), &model::Document::current_time_changed,
                item, [item]{item->update();}
            );
            return item;
        },
        [](model::PreCompLayer* shape){
            auto v = std::make_unique<GraphicsEditor>(shape);
            v->add_child<graphics::TransformGraphicsItem>(shape->transform.get(), shape, nullptr);
            return v;
        }
    );
    register_builder<model::TextShape>(
        &make_graphics_item_shape,
        [](model::TextShape* text){
            auto v = std::make_unique<GraphicsEditor>(text);
            v->add_child<graphics::TextAttributesEditor>(text);
            return v;
        }
    );
}

graphics::DocumentNodeGraphicsItem * graphics::GraphicsItemFactory::make_graphics_item(model::VisualNode* node) const
{
    if ( auto builder = builder_for(node) )
        return builder->make_graphics_item(node);
    return make_graphics_item_default(node);
}

std::unique_ptr<graphics::GraphicsEditor> graphics::GraphicsItemFactory::make_graphics_editor(model::VisualNode* node) const
{
    if ( auto builder = builder_for(node) )
        return builder->make_graphics_editor(node);
    return make_graphics_editor_default(node);
}

graphics::DocumentNodeGraphicsItem * graphics::GraphicsItemFactory::make_graphics_item_default(model::VisualNode* node)
{
    return new DocumentNodeGraphicsItem(node);
}

std::unique_ptr<graphics::GraphicsEditor> graphics::GraphicsItemFactory::make_graphics_editor_default(model::VisualNode*)
{
    return {};
}

graphics::GraphicsItemFactory::AbstractBuilder * graphics::GraphicsItemFactory::builder_for(model::VisualNode* node) const
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
