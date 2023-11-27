/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QBuffer>
#include <QImage>
#include <QPainter>

#include "io/mime/mime_serializer.hpp"
#include "io/io_registry.hpp"
#include "model/document.hpp"
#include "model/shapes/image.hpp"
#include "model/assets/assets.hpp"

namespace glaxnimate::io::raster {

class RasterMime : public io::mime::MimeSerializer
{
public:
    QString slug() const override { return "raster"_qs; }
    QString name() const override { return QObject::tr("Raster Image"); }
    QStringList mime_types() const override { return {"image/png"_qs}; }

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

    static QImage to_image(const std::vector<model::DocumentNode*>& selection)
    {
        if ( selection.empty() )
            return {};

        std::vector<model::VisualNode*> visual_nodes;
        visual_nodes.reserve(selection.size());
        QRectF box;

        for ( auto node : selection )
        {
            if ( auto visual = node->cast<model::VisualNode>() )
            {
                visual_nodes.push_back(visual);
                box |= visual->local_bounding_rect(visual->time());
            }
        }

        QImage image(box.size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-box.topLeft());
        for ( auto visual : visual_nodes )
        {
            visual->paint(&painter, visual->time(), model::VisualNode::Render);
        }
        return image;
    }

    static QImage frame_to_image(const model::VisualNode* node, model::FrameTime time)
    {
        if ( !node )
            return {};

        QImage image(node->local_bounding_rect(time).size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        node->paint(&painter, time, model::VisualNode::Render);
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
        out.main->shapes.insert(std::move(img));
        return out;
    }

private:
    static Autoreg<RasterMime> autoreg;
};

} // namespace glaxnimate::io::mime

