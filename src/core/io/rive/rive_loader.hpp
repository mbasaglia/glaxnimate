#pragma once

#include "rive_stream.hpp"
#include "type_def.hpp"

#include "rive_format.hpp"

namespace glaxnimate::io::rive {

class RiveLoader
{
public:
    RiveLoader(RiveStream& stream, RiveFormat* format);

    std::vector<Object> load_object_list();

    bool load_document(model::Document* document);

private:
    Object read_object();

    QVariant read_property_value(PropertyType type);


    model::Document* document;
    RiveStream& stream;
    RiveFormat* format;
    PropertyTable extra_props;
};

} // namespace glaxnimate::io::rive


