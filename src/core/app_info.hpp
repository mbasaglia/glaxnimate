#pragma once

#include <QDir>
#include <QString>
#include <QList>


class AppInfo
{
public:
    static AppInfo& instance()
    {
        static AppInfo singleton;
        return singleton;
    }

    /**
     * \brief Project machine-readable name
     */
    QString slug() const;

    /**
     * \brief Project machine-readable org name
     */
    QString organization() const;

    /**
     * \brief Project version
     */
    QString version() const;

    /**
     * \brief Project human-readable name
     */
    QString name() const;

    /**
     * \brief Documentation URL
     */
    QUrl url_docs() const;

    /**
     * \brief Bug reporting URL
     */
    QUrl url_issues() const;

private:
    AppInfo() = default;
    ~AppInfo() = default;

};