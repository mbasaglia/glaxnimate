#pragma once

#include "app/settings/setting.hpp"
#include "model/document.hpp"

#include <QFileInfo>
#include <QObject>
#include <QBuffer>

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
     * @pre @p file is open appropriately && @p setting_values contains all the settings correctly && can_open()
     */
    bool open(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        bool ok = on_open(file, filename, document, setting_values);
        document->set_has_file(true);
        emit completed(ok);
        return ok;
    }

    /**
     * @pre @p file is open appropriately && @p setting_values contains all the settings correctly && can_open()
     */
    bool save(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        bool ok = on_save(file, filename, document, setting_values);
        document->set_has_file(true);
        emit completed(ok);
        return ok;
    }

    Q_INVOKABLE QByteArray save(model::Document* document, const QVariantMap& setting_values={}, const QString& filename = "data");


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


protected:
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
    void error(const QString& message);
    void progress_max_changed(int max);
    void progress(int value);
    void completed(bool success);
};

} // namespace io

