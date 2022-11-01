#include "rive_html_format.hpp"
#include "rive_exporter.hpp"
#include "io/lottie/lottie_html_format.hpp"
#include "rive_format.hpp"


bool glaxnimate::io::rive::RiveHtmlFormat::on_save(
    QIODevice& file, const QString&, model::Document* document, const QVariantMap&
)
{
    file.write(lottie::LottieHtmlFormat::html_head(this, document,
        "<script src='https://unpkg.com/@rive-app/canvas@1.0.79'></script>"
    ));
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    RiveExporter exp(&buffer, this, RiveFormat::version_maj, RiveFormat::version_min);
    exp.write_document(document);
    file.write(QString(R"(
<body>
<canvas id="animation" width="%1" height="%2"></canvas>

<script>
    var rive_data = new Uint8Array([)")
        .arg(document->main()->width.get())
        .arg(document->main()->height.get())
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
