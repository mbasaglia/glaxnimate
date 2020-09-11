#pragma once

#include <QIODevice>
#include "model/shapes/shape.hpp"
#include "model/layers/layer.hpp"

namespace model {
    class MainComposition;
} // namespace model

namespace rendering {

class InkscapeSvgRenderer
{
public:
    InkscapeSvgRenderer(QIODevice* device);
    ~InkscapeSvgRenderer();

    void write_document(model::Document* document);
    void write_composition(model::Composition* comp);
    void write_main_composition(model::MainComposition* comp);
    void write_layer(model::Layer* layer);
    void write_shape(model::ShapeElement* shape);
    void write_node(model::DocumentNode* node);

    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace rendering
