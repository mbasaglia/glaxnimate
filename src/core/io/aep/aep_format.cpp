#include "aep_parser.hpp"
#include "aep_format.hpp"

glaxnimate::io::Autoreg<glaxnimate::io::aep::AepFormat> glaxnimate::io::aep::AepFormat::autoreg;
glaxnimate::io::Autoreg<glaxnimate::io::aep::AepxFormat> glaxnimate::io::aep::AepxFormat::autoreg;


bool glaxnimate::io::aep::AepFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& options)
{
}

bool glaxnimate::io::aep::AepxFormat::on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& options)
{
}
