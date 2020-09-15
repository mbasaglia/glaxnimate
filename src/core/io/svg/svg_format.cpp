#include "svg_format.hpp"

#include <QFileInfo>

#include "utils/gzip.hpp"
#include "svg_parser.hpp"

bool io::svg::SvgFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& )
{
    /// \todo layer mode setting
    SvgParser::GroupMode mode = SvgParser::Inkscape;

    auto on_error = [this](const QString& s){error(s);};
    try
    {
        if ( utils::gzip::is_compressed(file) )
        {
            QByteArray decompressed;
            if ( !utils::gzip::decompress(file, decompressed, on_error) )
                return false;
            QBuffer buffer(&decompressed);
            buffer.open(QIODevice::ReadOnly);
            SvgParser(&buffer, mode, document, document->main_composition()).parse_to_document(on_error);
            return true;
        }

        SvgParser(&file, mode, document, document->main_composition()).parse_to_document(on_error);
        return true;

    }
    catch ( const SvgParseError& err )
    {
        error(err.formatted(QFileInfo(filename).baseName()));
        return false;
    }
}

io::Autoreg<io::svg::SvgFormat> io::svg::SvgFormat::autoreg;
