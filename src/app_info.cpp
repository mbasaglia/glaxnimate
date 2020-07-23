#include "app_info.hpp"
#include "application_info_generated.hpp"

#include <QObject>

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
