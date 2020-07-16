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
     * \brief A path to write user preferences into
     * \param name Name of the data subdirectory
     */
    QString writable_data_path(const QString& name) const;

    /**
     * \brief Get all available directories to search data from
     * \param name Name of the data directory
     */
    QStringList data_paths(const QString& name) const;

    /**
     * \brief Get all directories to search data from
     *
     * This function may include directories that don't exist but that will be
     * checked if they existed
     *
     * \param name Name of the data directory
     */
    QStringList data_paths_unchecked(const QString& name) const;

    /**
     * \brief Get all possible directories to search data from
     */
    QList<QDir> data_roots() const;

private:
    AppInfo() = default;
    ~AppInfo() = default;

};
