#pragma once

#include <QIODevice>
#include "model/shapes/shape.hpp"

namespace model {
    class MainComposition;
} // namespace model

namespace io::svg {

enum AnimationType
{
    NotAnimated,
//     SMIL
};

class SvgRenderer
{
public:
    SvgRenderer(QIODevice* device, AnimationType animated);
    ~SvgRenderer();

    void write_document(model::Document* document);
    void write_composition(model::Composition* comp);
    void write_main(model::MainComposition* comp);
    void write_shape(model::ShapeElement* shape);
    void write_node(model::DocumentNode* node);

    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace io::svg
