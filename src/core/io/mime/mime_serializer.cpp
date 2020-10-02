#include "mime_serializer.hpp"

#include "app/log/log.hpp"

#include "model/object.hpp"
#include "json_mime.hpp"
#include "raster_mime.hpp"
#include "io/svg/svg_mime.hpp"
#include "model/document.hpp"


io::Autoreg<io::mime::JsonMime> io::mime::JsonMime::autoreg;
io::Autoreg<io::mime::RasterMime> io::mime::RasterMime::autoreg;
io::Autoreg<io::svg::SvgMime> io::svg::SvgMime::autoreg;


io::mime::DeserializedData io::mime::MimeSerializer::from_mime_data(
    const QMimeData& data,
    model::Document* owner_document
) const
{
    if ( !can_deserialize() )
        return {};

    for ( const QString& mime : mime_types() )
        if ( data.hasFormat(mime) )
            return deserialize(data.data(mime), owner_document);

    return {};
}

void io::mime::MimeSerializer::message(const QString& message, app::log::Severity severity) const
{
    app::log::Log(slug()).log(message, severity);
}


io::mime::DeserializedData io::mime::MimeSerializer::deserialize(
    const QByteArray&,
    model::Document*
) const
{
    return {};
}

io::mime::DeserializedData::DeserializedData() = default;
io::mime::DeserializedData::DeserializedData(io::mime::DeserializedData &&) = default;
io::mime::DeserializedData & io::mime::DeserializedData::operator=(io::mime::DeserializedData &&) = default;
io::mime::DeserializedData::~DeserializedData() = default;

bool io::mime::DeserializedData::empty() const
{
    return !document || document->main()->shapes.empty();
}


void io::mime::DeserializedData::initialize_data()
{
    document = std::make_unique<model::Document>("");
}
