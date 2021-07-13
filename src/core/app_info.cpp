#include "app_info.hpp"

#include <QGuiApplication>

#include "application_info_generated.hpp"

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

QString AppInfo::organization() const
{
    return PROJECT_SLUG;
}

QUrl AppInfo::url_docs() const
{
    return QUrl(URL_DOCS);
}

QUrl AppInfo::url_issues() const
{
    return QUrl(URL_ISSUES);
}

QString AppInfo::description() const
{
    return PROJECT_DESCRIPTION;
}

QUrl AppInfo::url_donate() const
{
    return QUrl(URL_DONATE);
}

void AppInfo::init_qapplication() const
{
    qApp->setApplicationName(slug());
    qApp->setApplicationVersion(version());
    qApp->setOrganizationName(organization());
    qApp->setApplicationDisplayName(name());
}
