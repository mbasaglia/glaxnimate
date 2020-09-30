#include "svg_format.hpp"

#include <QFileInfo>

#include "utils/gzip.hpp"
#include "svg_parser.hpp"

bool io::svg::SvgFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& )
{
    /// \todo layer mode setting
    SvgParser::GroupMode mode = SvgParser::Inkscape;

    auto on_error = [this](const QString& s){warning(s);};
    try
    {
        if ( utils::gzip::is_compressed(file) )
        {
            utils::gzip::GzipStream decompressed(&file, on_error);
            decompressed.open(QIODevice::ReadOnly);
            SvgParser(&decompressed, mode, document, document->main(), on_error).parse_to_document();
            return true;
        }

        SvgParser(&file, mode, document, document->main(), on_error).parse_to_document();
        return true;

    }
    catch ( const SvgParseError& err )
    {
        error(err.formatted(QFileInfo(filename).baseName()));
        return false;
    }
}

io::Autoreg<io::svg::SvgFormat> io::svg::SvgFormat::autoreg;
