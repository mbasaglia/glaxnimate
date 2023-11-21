/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QtGlobal>
#include <QIcon>
#include <QMimeData>

#include "app/application.hpp"


#ifdef MOBILE_UI

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
#ifdef Q_OS_ANDROID
    QString data_file(const QString& name) const override;
#endif
    static qreal handle_size_multiplier();
    static qreal handle_distance_multiplier();

    void set_clipboard_data(QMimeData* data);
    const QMimeData* get_clipboard_data();


    QString backup_path() const;

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

    const std::vector<app::log::LogLine>& log_lines() const
    {
        return store_logger->lines();
    }

    static GlaxnimateApp* instance()
    {
        return static_cast<GlaxnimateApp *>(QCoreApplication::instance());
    }

    QString backup_path() const;

    app::settings::ShortcutSettings* shortcuts() const;

    static QString temp_path();

    static qreal handle_size_multiplier() { return 1; }
    static qreal handle_distance_multiplier() { return 1; }


    void set_clipboard_data(QMimeData* data);
    const QMimeData* get_clipboard_data();

protected:
    void on_initialize() override;
    void on_initialize_settings() override;
    void on_initialize_translations() override;
    bool event(QEvent *event) override;

private:
    app::log::ListenerStore* store_logger;
    app::settings::ShortcutSettings* shortcut_settings;

};

} // namespace glaxnimate::gui
#endif
