#pragma once

#include <QFileInfo>
#include <QObject>
#include <QBuffer>

#include "app/settings/setting.hpp"
#include "app/log/log_line.hpp"

#include "model/document.hpp"

namespace io {

using Setting = app::settings::Setting;
using SettingList = app::settings::SettingList;

class ImportExport : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QStringList extensions READ extensions)
    Q_PROPERTY(bool can_open READ can_open)
    Q_PROPERTY(bool can_save READ can_save)

public:
    virtual ~ImportExport() = default;

    Q_INVOKABLE bool can_handle_extension(const QString& extension) const
    {
        return extensions().contains(extension);
    }

    Q_INVOKABLE bool can_handle_filename(const QString& filename) const
    {
        return can_handle_extension(QFileInfo(filename).completeSuffix());
    }

    /**
     * @pre @p setting_values contains all the settings correctly && can_open()
     */
    bool open(QIODevice& file, const QString& filename,
        model::Document* document, const QVariantMap& setting_values);

    /**
     * @pre @p setting_values contains all the settings correctly && can_open()
     */
    bool save(QIODevice& file, const QString& filename,
        model::Document* document, const QVariantMap& setting_values);

    Q_INVOKABLE QByteArray save(model::Document* document, const QVariantMap& setting_values={}, const QString& filename = "data");
    Q_INVOKABLE bool load(model::Document* document, const QByteArray& data, const QVariantMap& setting_values={}, const QString& filename = "data");


    virtual QString name() const = 0;
    virtual QStringList extensions() const = 0;
    virtual SettingList open_settings() const { return {}; }
    virtual SettingList save_settings() const { return {}; }
    virtual bool can_open() const = 0;
    virtual bool can_save() const = 0;

    /**
     * \brief File dialog name filter
     */
    Q_INVOKABLE QString name_filter() const;

    Q_INVOKABLE void warning(const QString& message)
    {
        emit this->message(message, app::log::Warning);
    }

    Q_INVOKABLE void information(const QString& message)
    {
        emit this->message(message, app::log::Info);
    }

    Q_INVOKABLE void error(const QString& message)
    {
        emit this->message(message, app::log::Error);
    }

protected:
    virtual bool auto_open() const { return true; }

    virtual bool on_open(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        Q_UNUSED(file);
        Q_UNUSED(filename);
        Q_UNUSED(document);
        Q_UNUSED(setting_values);
        return false;
    }

    virtual bool on_save(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        Q_UNUSED(file);
        Q_UNUSED(filename);
        Q_UNUSED(document);
        Q_UNUSED(setting_values);
        return false;
    }

signals:
    void message(const QString& message, app::log::Severity severity);
    void progress_max_changed(int max);
    void progress(int value);
    void completed(bool success);
};

} // namespace io

