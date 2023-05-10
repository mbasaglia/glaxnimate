/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "base.hpp"

QString glaxnimate::io::ImportExport::name_filter() const
{
    QString ext_str;
    for ( const QString& ext : extensions() )
    {
        ext_str += "*." + ext + " ";
    }

    if ( ext_str.isEmpty() )
        return {};

    ext_str.resize(ext_str.size() - 1);
    //: Open/Save file dialog file filter eg: "Text files (.txt)"
    return tr("%1 (%2)").arg(name()).arg(ext_str);
}


QByteArray glaxnimate::io::ImportExport::save(model::Composition* comp, const QVariantMap& setting_values, const QString& filename)
{
    QByteArray data;
    QBuffer file(&data);
    file.open(QIODevice::WriteOnly);

    QVariantMap clean_setting_values = setting_values;

    if ( auto settings = save_settings(comp) )
    {
        for ( const auto& setting : *settings )
            clean_setting_values[setting.slug] = setting.get_variant(clean_setting_values);
    }

    if ( !save(file, filename, comp, clean_setting_values) )
        return {};

    return data;
}


bool glaxnimate::io::ImportExport::open(QIODevice& file, const QString& filename,
                    model::Document* document, const QVariantMap& setting_values)
{
    if ( !file.isOpen() && auto_open() )
        if ( !file.open(QIODevice::ReadOnly) )
            return false;

    bool ok = on_open(file, filename, document, setting_values);
    emit completed(ok);
    return ok;
}

bool glaxnimate::io::ImportExport::save(QIODevice& file, const QString& filename,
                    model::Composition* comp, const QVariantMap& setting_values)
{
    if ( !file.isOpen() && auto_open() )
        if ( !file.open(QIODevice::WriteOnly) )
            return false;

    bool ok = on_save(file, filename, comp, setting_values);
    emit completed(ok);
    return ok;
}

bool glaxnimate::io::ImportExport::load(model::Document* document, const QByteArray& data,
                            const QVariantMap& setting_values, const QString& filename)
{
    if ( !document )
        return false;

    QBuffer file(const_cast<QByteArray*>(&data));
    file.open(QIODevice::ReadOnly);

    QVariantMap clean_setting_values = setting_values;

    if ( auto settings = open_settings() )
    {
        for ( const auto& setting : *settings )
            clean_setting_values[setting.slug] = setting.get_variant(clean_setting_values);
    }

    return open(file, filename, document, clean_setting_values);
}
