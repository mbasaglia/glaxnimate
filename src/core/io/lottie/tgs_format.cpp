#include "tgs_format.hpp"

#include <set>

#include "cbor_write_json.hpp"
#include "utils/gzip.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/repeater.hpp"
#include "model/visitor.hpp"

namespace glaxnimate::io::lottie {

class TgsVisitor : public model::Visitor
{
public:
    TgsVisitor(TgsFormat* fmt) : fmt(fmt) {}

private:
    void show_error(model::DocumentNode * node, const QString& message, app::log::Severity severity)
    {
        fmt->message(TgsFormat::tr("%1: %2").arg(node->object_name()).arg(message), severity);
    }

    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::PolyStar*>(node) )
        {
            show_error(node, TgsFormat::tr("Star Shapes are not officially supported"), app::log::Info);
        }
        else if ( qobject_cast<model::Image*>(node) )
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
    }

    void on_visit(model::Document * document) override
    {
        qreal width = document->main()->height.get();
        if ( width != 512 )
            fmt->error(TgsFormat::tr("Invalid width: %1, should be 512").arg(width));

        qreal height = document->main()->height.get();
        if ( height != 512 )
            fmt->error(TgsFormat::tr("Invalid height: %1, should be 512").arg(height));

        qreal fps = document->main()->fps.get();
        if ( fps != 30 && fps != 60 )
            fmt->error(TgsFormat::tr("Invalid fps: %1, should be 30 or 60").arg(fps));

        auto duration = document->main()->animation->duration();
        if ( duration > 180 )
            fmt->error(TgsFormat::tr("Too many frames: %1, should be less than 180"));
    }

    TgsFormat* fmt;
};

} // namespace glaxnimate::io::lottie

bool glaxnimate::io::lottie::TgsFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QByteArray json;
    if ( !utils::gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
        return false;
    return load_json(json, document);
}

bool glaxnimate::io::lottie::TgsFormat::on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    validate(document);

    QCborMap json = LottieFormat::to_json(document, true);
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


void glaxnimate::io::lottie::TgsFormat::validate(model::Document* document)
{
    TgsVisitor(this).visit(document);
}


glaxnimate::io::Autoreg<glaxnimate::io::lottie::TgsFormat> glaxnimate::io::lottie::TgsFormat::autoreg = {};
