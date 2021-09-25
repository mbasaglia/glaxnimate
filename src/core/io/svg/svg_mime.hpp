#pragma once

#include "io/svg/svg_parser.hpp"
#include "io/svg/svg_renderer.hpp"
#include "io/mime/mime_serializer.hpp"

namespace glaxnimate::io::svg {

class SvgMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "svg"; }
    QString name() const override { return QObject::tr("SVG"); }
    QStringList mime_types() const override { return {"image/svg+xml"}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        io::svg::SvgRenderer svg_rend(io::svg::NotAnimated, io::svg::CssFontType::FontFace);
        for ( auto node : selection )
            svg_rend.write_node(node);
        return svg_rend.dom().toByteArray(0);
    }

    io::mime::DeserializedData deserialize(const QByteArray& data) const override
    {
        QBuffer buffer(const_cast<QByteArray*>(&data));
        buffer.open(QIODevice::ReadOnly);

        auto on_error = [this](const QString& s){message(s);};
        try {
            return io::svg::SvgParser(&buffer, deserialize_group_mode, nullptr, on_error)
                .parse_to_objects();
        } catch ( const io::svg::SvgParseError& err ) {
            message(err.formatted("Clipboard"));
            return {};
        }
    }

    bool can_deserialize() const override { return true; }

    /// \todo show in settings
    io::svg::SvgParser::GroupMode deserialize_group_mode = io::svg::SvgParser::Inkscape;

private:
    static Autoreg<SvgMime> autoreg;
};

} // namespace glaxnimate::io::svg
