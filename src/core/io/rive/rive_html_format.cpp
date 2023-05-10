/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rive_html_format.hpp"
#include "rive_exporter.hpp"
#include "io/lottie/lottie_html_format.hpp"


bool glaxnimate::io::rive::RiveHtmlFormat::on_save(
    QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&
)
{
    file.write(lottie::LottieHtmlFormat::html_head(this, comp,
        "<script src='https://unpkg.com/@rive-app/canvas@1.0.79'></script>"
    ));
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    RiveExporter exp(&buffer, this);
    exp.write_document(comp->document());
    file.write(QString(R"(
<body>
<canvas id="animation" width="%1" height="%2"></canvas>

<script>
    var rive_data = new Uint8Array([)")
        .arg(comp->width.get())
        .arg(comp->height.get())
        .toUtf8()
    );

    for ( auto c : buffer.buffer() )
    {
        file.write(QString::number(c).toUtf8());
        file.write(",");
    }

    file.write(R"(]);

    var anim = new rive.Rive({
        buffer: rive_data,
        canvas: document.getElementById("animation"),
        autoplay: true
    });
</script>
</body></html>
)");

    return true;
}
