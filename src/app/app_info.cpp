#include "app_info.hpp"
#include "app/application_info_generated.hpp"

#include <QObject>
#include <QStandardPaths>
#include <QCoreApplication>

QString AppInfo::name() const
{
    return QObject::tr("Glaxnimate");
}

QString AppInfo::slug() const
{
    return PROJECT_SLUG;
}

QString AppInfo::version() const
{
    return PROJECT_VERSION;
}

QString AppInfo::writable_data_path(const QString& name) const
{
    QString search = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    if ( !search.isEmpty() )
    {
        return QDir::cleanPath(QDir(search).absoluteFilePath(name));
    }

    return QString();
}

QStringList AppInfo::data_paths(const QString& name) const
{
    QStringList found;

    for ( const QDir& d: data_roots() )
    {
        if ( d.exists() )
            found << QDir::cleanPath(d.absoluteFilePath(name));;
    }
    found.removeDuplicates();

    return found;
}

QStringList AppInfo::data_paths_unchecked(const QString& name) const
{
    QStringList filter;
    for ( const QDir& d: data_roots() )
    {
        filter << QDir::cleanPath(d.absoluteFilePath(name));
    }
    filter.removeDuplicates();

    return filter;
}

QList<QDir> AppInfo::data_roots() const
{
    QList<QDir> search;
    // std paths
    for ( const QString& str : QStandardPaths::standardLocations(QStandardPaths::DataLocation) )
        search.push_back(QDir(str));
    // executable dir
    QDir binpath(QCoreApplication::applicationDirPath());
    binpath.cdUp();
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(organization()).arg(slug())));

    return search;
}

QString AppInfo::organization() const
{
    return PROJECT_SLUG;
}
