#include "tgs_format.hpp"

#include <set>

#include "cbor_write_json.hpp"
#include "utils/gzip.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/stroke.hpp"
#include "model/visitor.hpp"

namespace io::lottie {

class TgsVisitor : public model::Visitor
{
public:
    TgsVisitor(TgsFormat* fmt) : fmt(fmt) {}

private:
    enum TreeError
    {
        StarShape,
        Image,
        GradientStroke,
    };

    void show_error(TreeError code)
    {
        if ( shown.count(code) )
            return;

        shown.insert(code);
        switch ( code )
        {
            case StarShape:
                fmt->information(TgsFormat::tr("Star Shapes are not officially supported"));
                break;
            case Image:
                fmt->error(TgsFormat::tr("Images are not supported"));
                break;
            case GradientStroke:
                fmt->information(TgsFormat::tr("Gradient strokes are not officially supported"));
                break;
        }
    }

    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::PolyStar*>(node) )
        {
            show_error(StarShape);
        }
        else if ( qobject_cast<model::Image*>(node) )
        {
            show_error(Image);
        }
        else if ( auto st = qobject_cast<model::Stroke*>(node) )
        {
            if ( qobject_cast<model::Gradient*>(st->use.get()) )
                show_error(GradientStroke);
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
    std::set<TreeError> shown;
};

} // namespace io::lottie

bool io::lottie::TgsFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    QByteArray json;
    if ( !utils::gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
        return false;
    return load_json(json, document);
}

bool io::lottie::TgsFormat::on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
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


void io::lottie::TgsFormat::validate(model::Document* document)
{
    TgsVisitor(this).visit(document);
}


io::Autoreg<io::lottie::TgsFormat> io::lottie::TgsFormat::autoreg = {};
