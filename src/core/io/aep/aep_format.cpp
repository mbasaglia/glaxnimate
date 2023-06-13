#include "aep_format.hpp"
#include "aep_parser.hpp"
#include "aep_loader.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::aep::AepFormat> glaxnimate::io::aep::AepFormat::autoreg;
glaxnimate::io::Autoreg<glaxnimate::io::aep::AepxFormat> glaxnimate::io::aep::AepxFormat::autoreg;


bool glaxnimate::io::aep::AepFormat::riff_to_document(const RiffChunk& chunk, model::Document* document, const QString& filename)
{
    AepParser parser(this);
    try {
        Project project = parser.parse(chunk);
        QFileInfo finfo(filename);
        AepLoader loader(document, project, finfo.dir(), this);
        loader.load_project();
        return true;
    } catch ( const AepError& err ) {
        return false;
    }
}

bool glaxnimate::io::aep::AepFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap&)
{
    AepRiff riff_parser;
    try {
        RiffChunk chunk = riff_parser.parse(&file);
        return riff_to_document(chunk, document, filename);
    } catch ( const RiffError& r ) {
        error(tr("Could not load file: %1").arg(r.message));
        return false;
    }
}

bool glaxnimate::io::aep::AepxFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap&)
{
}
