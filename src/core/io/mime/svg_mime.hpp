#pragma once

#include <QBuffer>

#include "io/svg/inkscape_svg.hpp"
#include "io/mime/mime_serializer.hpp"

namespace io::mime {

class SvgMime : public io::mime::MimeSerializer
{
public:
    QString name() const override { return QObject::tr("SVG"); }
    QStringList mime_types() const override { return {"image/svg+xml"}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        io::svg::InkscapeSvgRenderer svg_rend(&buffer);
        for ( auto node : selection )
            svg_rend.write_node(node);
        svg_rend.close();
        return data;
    }

    std::vector<std::unique_ptr<model::DocumentNode>> deserialize(
        const QByteArray&,
        model::Document*,
        model::Composition*
    ) const override { return {}; }

    bool can_deserialize() const override { return false; }
};

} // namespace io::mime
