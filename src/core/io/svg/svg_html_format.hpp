#pragma once

#include "io/lottie/lottie_html_format.hpp"
#include "io/svg/svg_format.hpp"

namespace glaxnimate::io::svg {

class SvgHtmlFormat : public SvgFormat
{
public:
    QString slug() const override { return "svg_html"; }
    QString name() const override { return SvgFormat::tr("SVG Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override
    {
        file.write(lottie::LottieHtmlFormat::html_head(this, document, {}));
        file.write("<body><div id='animation'>");
        bool ok = SvgFormat::on_save(file, filename, document, setting_values);
        file.write("</div></body></html>");
        return ok;
    }
};

} // namespace glaxnimate::io::svg
