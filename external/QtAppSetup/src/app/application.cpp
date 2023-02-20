/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "application.hpp"

#include <QStandardPaths>
#include <QMetaEnum>
#include <QIcon>

#include "app/log/log.hpp"


void app::Application::initialize()
{
    on_initialize();
    on_initialize_translations();
    on_initialize_settings();
    app::settings::Settings::instance().load();
}

QString app::Application::writable_data_path(const QString& name) const
{
    QString search = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

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
        if ( d.exists(name) )
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
    for ( const QString& str : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation) )
        search.push_back(QDir(str));
    // executable dir
    QDir binpath(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
    // some Windows apps do not use a bin subfolder
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(organizationName()).arg(applicationName())));
#endif
    binpath.cdUp();
    search.push_back(binpath.filePath(QString("share/%1/%2").arg(organizationName()).arg(applicationName())));
#ifdef Q_OS_MAC
    // some macOS app bundles use a Resources subfolder
    search.push_back(binpath.filePath(QString("Resources/%1/%2").arg(organizationName()).arg(applicationName())));
#endif

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

void app::Application::on_initialize_translations()
{
    app::TranslationService::instance().initialize();
}
