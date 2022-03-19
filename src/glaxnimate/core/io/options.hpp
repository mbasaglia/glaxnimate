#pragma once

#include <QDir>
#include <QVariant>

namespace glaxnimate::io {


class ImportExport;

struct Options
{
    ImportExport* format = nullptr;
    QDir path;
    QString filename;
    QVariantMap settings;

    void clear()
    {
        *this = {};
    }
};


} // namespace glaxnimate::io
