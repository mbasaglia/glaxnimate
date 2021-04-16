#pragma once

#include <QBuffer>
#include <QImage>
#include <QPainter>

#include "io/mime/mime_serializer.hpp"
#include "model/document.hpp"
#include "model/shapes/image.hpp"

namespace io::raster {

class RasterMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "raster"; }
    QString name() const override { return QObject::tr("Raster Image"); }
    QStringList mime_types() const override { return {"image/png"}; }

    QByteArray serialize(const std::vector<model::DocumentNode*>& selection) const override
    {
        QByteArray data;
        QBuffer buffer(&data);
        to_image(selection).save(&buffer, "PNG");
        return data;
    }

    bool can_deserialize() const override { return true; }

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
        {
            if ( auto visual = node->cast<model::VisualNode>() )
                visual->paint(&painter, node->time(), model::VisualNode::Render);
        }
        return image;
    }

    io::mime::DeserializedData deserialize(const QByteArray& data) const override
    {
        io::mime::DeserializedData out;
        out.initialize_data();
        auto bmp = out.document->assets()->images->values.insert(std::make_unique<model::Bitmap>(out.document.get()));
        bmp->data.set(data);
        auto img = std::make_unique<model::Image>(out.document.get());
        img->image.set(bmp);
        QPointF p(bmp->pixmap().width() / 2.0, bmp->pixmap().height() / 2.0);
        img->transform->anchor_point.set(p);
        img->transform->position.set(p);
        out.document->main()->shapes.insert(std::move(img));
        return out;
    }

private:
    static Autoreg<RasterMime> autoreg;
};

} // namespace io::mime

