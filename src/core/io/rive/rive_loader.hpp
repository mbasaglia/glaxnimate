#pragma once

#include "io/binary_stream.hpp"

#include "type_system.hpp"
#include "rive_format.hpp"

namespace glaxnimate::io::rive {


class RiveLoader
{
public:
    RiveLoader(BinaryInputStream& stream, RiveFormat* format);

    std::vector<Object> load_object_list();

    bool load_document(model::Document* document);

    const PropertyTable& extra_properties() const;

private:
    Object read_object();

    QVariant read_property_value(PropertyType type);
    PropertyTable read_property_table();
    void skip_value(PropertyType type);

    QByteArray read_raw_string();
    QString read_string_utf8();


    model::Document* document;
    BinaryInputStream& stream;
    RiveFormat* format;
    PropertyTable extra_props;
    TypeSystem types;
};

} // namespace glaxnimate::io::rive
