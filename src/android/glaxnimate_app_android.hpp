#pragma once

#include <QApplication>

class GlaxnimateApp : public QApplication
{
    Q_OBJECT

public:
    using QApplication::QApplication;

    static GlaxnimateApp* instance()
    {
        return static_cast<GlaxnimateApp *>(QCoreApplication::instance());
    }

    QString data_file(const QString& name) const
    {
        return ":/" + name;
    }
};
