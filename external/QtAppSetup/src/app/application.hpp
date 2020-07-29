#pragma once

#include <QApplication>
#include <QSettings>
#include <QDir>

#include "app/settings/settings.hpp"
#include "app/translation_service.hpp"

namespace app {

class Application : public QApplication
{
    Q_OBJECT

public:
    using QApplication::QApplication;

    virtual QSettings qsettings() const;

    virtual void load_settings_metadata() const {}

    void initialize()
    {
        on_initialize();
        app::TranslationService::instance().initialize();
        app::settings::Settings::instance().load();
    }

    void finalize()
    {
        app::settings::Settings::instance().save();
    }

    static Application* instance()
    {
        return static_cast<Application *>(QCoreApplication::instance());
    }


    /**
     * \brief A path to write user preferences into
     * \param name Name of the data subdirectory
     */
    QString writable_data_path(const QString& name) const;

    /**
     * \brief Path to get the file from
     * \param name Name of the data files
     */
    QString data_file(const QString& name) const;

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

protected:
    /**
     * \brief Called after construction, before anything else
     * \note set application name and stuff in here
     */
    virtual void on_initialize() {}
};

} // namespace app
