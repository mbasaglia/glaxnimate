#include "rive_html_format.hpp"
#include "rive_exporter.hpp"
#include "io/lottie/lottie_html_format.hpp"


bool glaxnimate::io::rive::RiveHtmlFormat::on_save(QIODevice& file, const QString&,
                                           model::Document* document, const QVariantMap& settings)
{
    file.write(lottie::LottieHtmlFormat::html_head(this, document,
        "<script src='https://unpkg.com/@rive-app/canvas@1.0.79'></script>"
    ));
    QBuffer buffer;
    RiveExporter exp(&buffer, this);
    exp.write_document(document);
    file.write(R"(
<body>
<div id="animation"></div>
<canvas id="animation" width="%1" height="%2"></canvas>

<script>
    var rive_data = new UInt8Array([)");

    for ( auto c : buffer.buffer() )
    {
        file.write(QString::number(c).toUtf8());
        file.write(",");
    }

file.write(QString(R"(]);
    ;

    var anim = new rive.Rive({
        buffer: rive_daya,
        canvas: document.getElementById("animation"),
        autoplay: true
    });
</script>
</body></html>
)")
        .arg(settings["renderer"].toString())
        .toUtf8()
    );

    return true;
}
