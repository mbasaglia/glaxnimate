#pragma once

#include <memory>

#include <QtGlobal>
#include <QIcon>
#include <QMimeData>

#include "app/application.hpp"


#ifdef Q_OS_ANDROID

namespace glaxnimate::gui {

class GlaxnimateApp : public app::Application
{
    Q_OBJECT

public:
    using app::Application::Application;

    static GlaxnimateApp* instance()
    {
        return static_cast<GlaxnimateApp *>(QCoreApplication::instance());
    }

    /// \todo remove, no longer needed
    static QIcon theme_icon(const QString& name);

    QString data_file(const QString& name) const override;

    static qreal handle_size_multiplier();
    static qreal handle_distance_multiplier();

    void set_clipboard_data(QMimeData* data);
    const QMimeData* get_clipboard_data();


    QString backup_path(const QString& file = {}) const;

    static QString temp_path();

private:
    std::unique_ptr<QMimeData> clipboard = std::make_unique<QMimeData>();
};

} // namespace glaxnimate::gui

#else

#include "app/log/listener_stderr.hpp"
#include "app/log/listener_store.hpp"

namespace app::settings { class ShortcutSettings; }

namespace glaxnimate::gui {

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

    static QIcon theme_icon(const QString& name)
    {
        return QIcon::fromTheme(name);
    }

    static qreal handle_size_multiplier() { return 1; }
    static qreal handle_distance_multiplier() { return 1; }


    void set_clipboard_data(QMimeData* data);
    const QMimeData* get_clipboard_data();

protected:
    void on_initialize() override;

private:
    app::log::ListenerStore* store_logger;
    app::settings::ShortcutSettings* shortcut_settings;

};

} // namespace glaxnimate::gui
#endif
