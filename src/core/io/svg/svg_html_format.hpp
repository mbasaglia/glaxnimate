/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "io/lottie/lottie_html_format.hpp"
#include "io/svg/svg_renderer.hpp"

namespace glaxnimate::io::svg {

class SvgHtmlFormat : public ImportExport
{
public:
    QString slug() const override { return "svg_html"; }
    QString name() const override { return QObject::tr("SVG Preview"); }
    QStringList extensions() const override { return {"html", "htm"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
    bool on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&) override
    {
        file.write(lottie::LottieHtmlFormat::html_head(this, comp, {}));
        file.write("<body><div id='animation'>");
        SvgRenderer rend(SMIL, CssFontType::FontFace);
        rend.write_main(comp);
        rend.write(&file, true);
        file.write("</div></body></html>");
        return true;

    }
};

} // namespace glaxnimate::io::svg
