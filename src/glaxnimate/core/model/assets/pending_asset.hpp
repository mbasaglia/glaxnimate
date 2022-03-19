#pragma once

#include <QUrl>
#include <QByteArray>


namespace glaxnimate::model {

struct PendingAsset
{
    int id = 0;
    QUrl url;
    QByteArray data;
    QString name_alias;
    bool loaded = false;
};

} // namespace glaxnimate::model
