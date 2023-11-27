/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lottie_html_format.hpp"
#include "lottie_exporter.hpp"
#include "cbor_write_json.hpp"

QByteArray glaxnimate::io::lottie::LottieHtmlFormat::html_head(ImportExport* ie,  model::Composition* comp, const QString& extra)
{
    return QStringLiteral(
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
    .arg(comp->width.get())
    .arg(comp->height.get())
    .arg(extra)
    .arg(comp->object_name())
    .arg(ie->name())
    .toUtf8()
    ;
}

bool glaxnimate::io::lottie::LottieHtmlFormat::on_save(QIODevice& file, const QString&,
                                                       model::Composition* comp, const QVariantMap& settings)
{
    const char* lottie_web_version = "5.12.2";
    file.write(html_head(this, comp,
        QStringLiteral("<script src='https://cdnjs.cloudflare.com/ajax/libs/bodymovin/%1/lottie.js'></script>").arg(QString::fromLatin1(lottie_web_version))
    ));
    file.write(R"(
<body>
<div id="animation"></div>

<script>
    var lottie_json = )");
    detail::LottieExporterState exp(this, comp, false, false, {{"auto_embed"_qs, true}});
    file.write(cbor_write_json(exp.to_json(), false));

file.write(QStringLiteral(R"(
    ;

    var anim = null;

    function reload()
    {
        var animData = {
            container: document.getElementById('animation'),
            renderer: '%1',
            loop: true,
            autoplay: true,
            animationData: lottie_json
        };
        if ( anim != null )
            anim = anim.destroy();
        anim = bodymovin.loadAnimation(animData);
    }

    reload();
</script>
</body></html>
)")
        .arg(settings["renderer"_qs].toString())
        .toUtf8()
    );

    return true;
}
