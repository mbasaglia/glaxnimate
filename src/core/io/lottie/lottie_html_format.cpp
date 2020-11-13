#include "lottie_html_format.hpp"
#include "lottie_format.hpp"
#include "cbor_write_json.hpp"

QByteArray io::lottie::LottieHtmlFormat::html_head(ImportExport* ie, model::Document* document, const QString& extra)
{
    return QString(
R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8" />
    <title>%4: %5</title>
    <style>
        html, body { width: 100%; height: 100%; margin: 0; }
        body { display: flex; }
        #animation { width: %1px; height: %2px; margin: auto;
            background-color: white;
            background-size: 64px 64px;
            background-image:
                linear-gradient(to right, rgba(0, 0, 0, .3) 50%, transparent 50%),
                linear-gradient(to bottom, rgba(0, 0, 0, .3) 50%, transparent 50%),
                linear-gradient(to bottom, white 50%, transparent 50%),
                linear-gradient(to right, transparent 50%, rgba(0, 0, 0, .5) 50%);
        }
    </style>
    %3
</head>
)")
    .arg(document->main()->width.get())
    .arg(document->main()->height.get())
    .arg(extra)
    .arg(document->main()->object_name())
    .arg(ie->name())
    .toUtf8()
    ;
}

bool io::lottie::LottieHtmlFormat::on_save(QIODevice& file, const QString&,
                                           model::Document* document, const QVariantMap&)
{
    file.write(html_head(this, document, "<script src='https://cdnjs.cloudflare.com/ajax/libs/bodymovin/5.5.3/lottie.js'></script>"));
    file.write(R"(
<body>
<div id="animation"></div>

<script>
    var animData = {
        container: document.getElementById('animation'),
        renderer: 'svg',
        loop: true,
        autoplay: true,
        animationData:
)");

    file.write(cbor_write_json(LottieFormat().to_json(document), false));

    file.write(R"(
    };
    var anim = bodymovin.loadAnimation(animData);
</script>
</body></html>
)");

    return true;
}
