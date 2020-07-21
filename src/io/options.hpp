#pragma once

#include <QDir>
#include <QVariant>

namespace io {


class ImportExport;

struct Options
{
    ImportExport* method = nullptr;
    QDir path;
    QString filename;
    QVariantMap settings;

    void clear()
    {
        *this = {};
    }
};


} // namespace io
