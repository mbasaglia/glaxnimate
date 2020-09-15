#pragma once

#include <QBuffer>
#include <QImage>
#include <QPainter>

#include "io/mime/mime_serializer.hpp"
#include "model/document.hpp"

namespace io::mime {

class RasterMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "raster"; }
    QString name() const override { return QObject::tr("Raster Image"); }
    QStringList mime_types() const override { return {}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        QByteArray data;
        QBuffer buffer(&data);
        to_image(selection).save(&buffer, "PNG");
        return data;
    }

    std::vector<std::unique_ptr<model::DocumentNode>> deserialize(
        const QByteArray&,
        model::Document*,
        model::Composition*
    ) const override { return {}; }

    bool can_deserialize() const override { return false; }

    void to_mime_data(QMimeData& mime, const std::vector<model::DocumentNode*>& objects) const override
    {
        mime.setImageData(to_image(objects));
    }

    QImage to_image(const std::vector<model::DocumentNode*>& selection) const
    {
        if ( selection.empty() )
            return {};

        QImage image(selection[0]->document()->size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        for ( auto node : selection )
            node->paint(&painter, node->time(), model::DocumentNode::Recursive);
        return image;
    }
private:
    static Autoreg<RasterMime> autoreg;
};

} // namespace io::mime

