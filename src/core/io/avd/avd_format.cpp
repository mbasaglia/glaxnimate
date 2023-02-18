#include "avd_format.hpp"

#include "avd_parser.hpp"
#include "io/svg/parse_error.hpp"
#include "avd_renderer.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::avd::AvdFormat> glaxnimate::io::avd::AvdFormat::autoreg;


bool glaxnimate::io::avd::AvdFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& options)
{
    auto on_error = [this](const QString& s){warning(s);};
    try
    {
        QSize forced_size = options["forced_size"].toSize();
        auto resource_path = QFileInfo(filename).dir();

        AvdParser(&file, resource_path, document, on_error, this, forced_size).parse_to_document();
        return true;

    }
    catch ( const svg::SvgParseError& err )
    {
        error(err.formatted(QFileInfo(filename).baseName()));
        return false;
    }
}

bool glaxnimate::io::avd::AvdFormat::on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    auto on_error = [this](const QString& s){warning(s);};
    AvdRenderer rend(on_error);
    rend.render(document);
    auto dom = rend.single_file();
    file.write(dom.toByteArray(4));
    return true;
}
