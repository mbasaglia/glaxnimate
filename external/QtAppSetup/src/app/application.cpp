#include "application.hpp"

#include <QStandardPaths>
#include <QMetaEnum>
#include <QIcon>

#include "app/log/log.hpp"


QString app::Application::writable_data_path(const QString& name) const
{
    QString search = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if ( !search.isEmpty() )
    {
        return QDir::cleanPath(QDir(search).absoluteFilePath(name));
    }

    return QString();
}

QStringList app::Application::data_paths(const QString& name) const
{
    QStringList found;

    for ( const QDir& d: data_roots() )
    {
        if ( d.exists() )
            found << QDir::cleanPath(d.absoluteFilePath(name));
    }
    found.removeDuplicates();

    return found;
}

QStringList app::Application::data_paths_unchecked(const QString& name) const
{
    QStringList filter;
    for ( const QDir& d: data_roots() )
    {
        filter << QDir::cleanPath(d.absoluteFilePath(name));
    }
    filter.removeDuplicates();

    return filter;
}

QList<QDir> app::Application::data_roots() const
{
    QList<QDir> search;
    // std paths
    for ( const QString& str : QStandardPaths::standardLocations(QStandardPaths::DataLocation) )
        search.push_back(QDir(str));
    // executable dir
    QDir binpath(QCoreApplication::applicationDirPath());
    binpath.cdUp();
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(organizationName()).arg(applicationName())));

    return search;
}

QString app::Application::data_file(const QString& name) const
{
    QStringList found;

    for ( const QDir& d: data_roots() )
    {
        if ( d.exists(name) )
            return QDir::cleanPath(d.absoluteFilePath(name));
    }

    return {};
}

QSettings app::Application::qsettings() const
{
    return QSettings(writable_data_path("settings.ini"), QSettings::IniFormat);
}

bool app::Application::notify(QObject* receiver, QEvent* e)
{
    try {
        return QApplication::notify(receiver, e);
    } catch ( const std::exception& exc ) {
        log::Log("Event", QMetaEnum::fromType<QEvent::Type>().valueToKey(e->type())).stream(log::Error) << "Exception:" << exc.what();
        return false;
    }
}
