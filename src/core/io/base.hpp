/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFileInfo>
#include <QObject>
#include <QBuffer>

#include "app/settings/settings_group.hpp"
#include "app/log/log_line.hpp"

#include "model/document.hpp"
#include "model/assets/composition.hpp"

namespace glaxnimate::io {

class ImportExport : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString slug READ slug)
    Q_PROPERTY(QStringList extensions READ extensions)
    Q_PROPERTY(bool can_open READ can_open)
    Q_PROPERTY(bool can_save READ can_save)

public:
    enum Direction
    {
        Import,
        Export,
    };
    Q_ENUM(Direction)

    virtual ~ImportExport() = default;

    Q_INVOKABLE bool can_handle(glaxnimate::io::ImportExport::Direction direction) const
    {
        if ( direction == Import )
            return can_open();
        else if ( direction == Export )
            return can_save();
        return false;
    }

    Q_INVOKABLE bool can_handle_extension(const QString& extension, glaxnimate::io::ImportExport::Direction direction) const
    {
        return can_handle(direction) && extensions().contains(extension);
    }

    Q_INVOKABLE bool can_handle_filename(const QString& filename, glaxnimate::io::ImportExport::Direction direction) const
    {
        return can_handle_extension(QFileInfo(filename).completeSuffix(), direction);
    }

    /**
     * @pre @p setting_values contains all the settings correctly && can_open()
     */
    bool open(QIODevice& file, const QString& filename,
        model::Document* document, const QVariantMap& setting_values);

    /**
     * @pre @p setting_values contains all the settings correctly && can_open()
     * @param file File to write to
     * @param filename Filename for error reporting
     * @param comp Composition, for formats supporting multiple comps, use comp->document()
     * @param setting_values Values based on save_settings()
     */
    bool save(QIODevice& file, const QString& filename,
        model::Composition* comp, const QVariantMap& setting_values);

    /**
     * \brief Will save the first comp in the document
     */
    bool save(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& setting_values);

    Q_INVOKABLE QByteArray save(glaxnimate::model::Composition* comp, const QVariantMap& setting_values={}, const QString& filename = "data"_qs);
    Q_INVOKABLE bool load(glaxnimate::model::Document* document, const QByteArray& data, const QVariantMap& setting_values={}, const QString& filename = "data"_qs);


    virtual QString name() const = 0;
    virtual QString slug() const = 0;
    virtual QStringList extensions() const = 0;
    virtual std::unique_ptr<app::settings::SettingsGroup> open_settings() const { return {}; }
    virtual std::unique_ptr<app::settings::SettingsGroup> save_settings(model::Composition* ) const { return {}; }
    virtual bool can_open() const = 0;
    virtual bool can_save() const = 0;
    /**
     * \brief Priority when multiple classes support the same file types
     */
    virtual int priority() const { return 0; }

    /**
     * \brief File dialog name filter
     */
    Q_INVOKABLE QString name_filter() const;

    Q_INVOKABLE void warning(const QString& message)
    {
        Q_EMIT this->message(message, app::log::Warning);
    }

    Q_INVOKABLE void information(const QString& message)
    {
        Q_EMIT this->message(message, app::log::Info);
    }

    Q_INVOKABLE void error(const QString& message)
    {
        Q_EMIT this->message(message, app::log::Error);
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

    virtual bool on_save(
        QIODevice& file,
        const QString& filename,
        model::Composition* comp,
        const QVariantMap& setting_values)
    {
        Q_UNUSED(file);
        Q_UNUSED(filename);
        Q_UNUSED(comp);
        Q_UNUSED(setting_values);
        return false;
    }

Q_SIGNALS:
    void message(const QString& message, app::log::Severity severity);
    void progress_max_changed(int max);
    void progress(int value);
    void completed(bool success);
};

} // namespace glaxnimate::io

