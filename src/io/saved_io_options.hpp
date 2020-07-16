#pragma once

#include <QDir>
#include <QVariant>

namespace io {


class ImportExport;

struct SavedIoOptions
{
    ImportExport* method = nullptr;
    QDir path;
    QString filename;
    QVariantMap options;

    void clear()
    {
        *this = {};
    }
};


} // namespace io
