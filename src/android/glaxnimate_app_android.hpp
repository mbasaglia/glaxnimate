#pragma once

#include <QApplication>
#include <QIcon>

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
        return ":" + applicationName() + "/" + name;
    }

    static QIcon theme_icon(const QString& name)
    {
        return QIcon(QString() + ":glaxnimate/images/icons/" + name + ".svg");
    }
};
