/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_info.hpp"

#include <QGuiApplication>
#include <KAboutData>

#include "application_info_generated.hpp"

QString glaxnimate::AppInfo::name() const
{
    return QObject::tr("Glaxnimate");
}

QString glaxnimate::AppInfo::slug() const
{
    return QStringLiteral(PROJECT_SLUG);
}

QString glaxnimate::AppInfo::version() const
{
    return QStringLiteral(PROJECT_VERSION);
}

QString glaxnimate::AppInfo::organization() const
{
    return QStringLiteral(PROJECT_SLUG);
}

QUrl glaxnimate::AppInfo::url_docs() const
{
    return QUrl(QStringLiteral(URL_DOCS));
}

QUrl glaxnimate::AppInfo::url_issues() const
{
    return QUrl(QStringLiteral(URL_ISSUES));
}

QString glaxnimate::AppInfo::description() const
{
    return QStringLiteral(PROJECT_DESCRIPTION);
}

QUrl glaxnimate::AppInfo::url_donate() const
{
    return QUrl(QStringLiteral(URL_DONATE));
}

void glaxnimate::AppInfo::init_qapplication() const
{
    qApp->setApplicationName(slug());
    qApp->setApplicationVersion(version());
    qApp->setOrganizationName(organization());
    qApp->setApplicationDisplayName(name());

    KAboutData aboutData(
        QStringLiteral(PROJECT_SLUG),
        name(),
        QStringLiteral(PROJECT_VERSION),
        QStringLiteral(PROJECT_DESCRIPTION),
        KAboutLicense::GPL,
        QObject::tr("(c) 2019-2023"),
        // Optional text shown in the About box.
        QStringLiteral(""),
        QStringLiteral(URL_DOCS),
        QStringLiteral(URL_ISSUES)
    );

    KAboutData::setApplicationData(aboutData);
}
