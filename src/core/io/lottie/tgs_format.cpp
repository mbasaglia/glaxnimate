/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tgs_format.hpp"

#include <set>

#include "cbor_write_json.hpp"
#include "utils/gzip.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/repeater.hpp"
#include "model/shapes/inflate_deflate.hpp"
#include "model/shapes/offset_path.hpp"
#include "model/shapes/zig_zag.hpp"
#include "validation.hpp"


using namespace glaxnimate;
using namespace glaxnimate::io::lottie;

namespace {

class TgsVisitor : public ValidationVisitor
{

public:
    explicit TgsVisitor(LottieFormat* fmt)
        : ValidationVisitor(fmt)
    {
        allowed_fps.push_back(30);
        allowed_fps.push_back(60);
        fixed_size = QSize(512, 512);
        max_frames = 180;
    }

private:
    using ValidationVisitor::on_visit;

    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::PolyStar*>(node) )
        {
            show_error(node, TgsFormat::tr("Star Shapes are not officially supported"), app::log::Info);
        }
        else if ( qobject_cast<model::Image*>(node) || qobject_cast<model::Bitmap*>(node) )
        {
            show_error(node, TgsFormat::tr("Images are not supported"), app::log::Error);
        }
        else if ( auto st = qobject_cast<model::Stroke*>(node) )
        {
            if ( qobject_cast<model::Gradient*>(st->use.get()) )
                show_error(node, TgsFormat::tr("Gradient strokes are not officially supported"), app::log::Info);
        }
        else if ( auto layer = qobject_cast<model::Layer*>(node) )
        {
            if ( layer->mask->has_mask() )
                show_error(node, TgsFormat::tr("Masks are not supported"), app::log::Error);
        }
        else if ( qobject_cast<model::Repeater*>(node) )
        {
            show_error(node, TgsFormat::tr("Repeaters are not officially supported"), app::log::Info);
        }
        else if ( qobject_cast<model::InflateDeflate*>(node) )
        {
            show_error(node, TgsFormat::tr("Inflate/Deflate is not supported"), app::log::Warning);
        }
        else if ( qobject_cast<model::OffsetPath*>(node) )
        {
            show_error(node, TgsFormat::tr("Offset Path is not supported"), app::log::Warning);
        }
        else if ( qobject_cast<model::ZigZag*>(node) )
        {
            show_error(node, TgsFormat::tr("ZigZag is not supported"), app::log::Warning);
        }
    }
};

} // namespace

bool glaxnimate::io::lottie::TgsFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QByteArray json;
    if ( !utils::gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
        return false;
    return load_json(json, document);
}

bool glaxnimate::io::lottie::TgsFormat::on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&)
{
    validate(comp->document(), comp);

    QCborMap json = LottieFormat::to_json(comp, true, true);
    json[QLatin1String("tgs")] = 1;
    QByteArray data = cbor_write_json(json, true);

    quint32 compressed_size = 0;
    if ( !utils::gzip::compress(data, file, [this](const QString& s){ error(s); }, 9, &compressed_size) )
        return false;

    qreal size_k = compressed_size / 1024.0;
    if ( size_k > 64 )
        error(tr("File too large: %1k, should be under 64k").arg(size_k));

    return true;
}


void glaxnimate::io::lottie::TgsFormat::validate(model::Document* document, model::Composition* comp)
{
    TgsVisitor(this).visit(document, comp);
}


glaxnimate::io::Autoreg<glaxnimate::io::lottie::TgsFormat> glaxnimate::io::lottie::TgsFormat::autoreg = {};
