#include "mime_serializer.hpp"

#include "app/log/log.hpp"

#include "model/document_node.hpp"
#include "json_mime.hpp"
#include "raster_mime.hpp"
#include "io/svg/svg_mime.hpp"


io::Autoreg<io::mime::JsonMime> io::mime::JsonMime::autoreg;
io::Autoreg<io::mime::RasterMime> io::mime::RasterMime::autoreg;
io::Autoreg<io::svg::SvgMime> io::svg::SvgMime::autoreg;


std::vector<std::unique_ptr<model::DocumentNode>> io::mime::MimeSerializer::from_mime_data(
    const QMimeData& data,
    model::Document* owner_document,
    model::Composition* owner_composition
) const
{
    if ( !can_deserialize() )
        return {};

    for ( const QString& mime : mime_types() )
        if ( data.hasFormat(mime) )
            return deserialize(data.data(mime), owner_document, owner_composition);

    return {};
}

void io::mime::MimeSerializer::message(const QString& message, app::log::Severity severity) const
{
    app::log::Log(slug()).log(message, severity);
}
