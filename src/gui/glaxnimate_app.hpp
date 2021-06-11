#pragma once
#include "app/application.hpp"
#include "app/log/listener_stderr.hpp"
#include "app/log/listener_store.hpp"


namespace app::settings { class ShortcutSettings; }

class GlaxnimateApp : public app::Application
{
    Q_OBJECT

public:
    using app::Application::Application;

    void load_settings_metadata() const override;

    const std::vector<app::log::LogLine>& log_lines() const
    {
        return store_logger->lines();
    }

    static GlaxnimateApp* instance()
    {
        return static_cast<GlaxnimateApp *>(QCoreApplication::instance());
    }

    QString backup_path(const QString& file = {}) const;

    app::settings::ShortcutSettings* shortcuts() const;

    static QString temp_path();

protected:
    void on_initialize() override;

private:
    app::log::ListenerStore* store_logger;
    app::settings::ShortcutSettings* shortcut_settings;

};
