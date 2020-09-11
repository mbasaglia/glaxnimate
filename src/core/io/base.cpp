#include "base.hpp"

QString io::ImportExport::name_filter() const
{
    QString ext_str;
    for ( const QString& ext : extensions() )
    {
        ext_str += ext + " ";
    }

    if ( ext_str.isEmpty() )
        return {};

    ext_str.resize(ext_str.size() - 1);
    //: Open/Save file dialog file filter eg: "Text files (.txt)"
    return tr("%1 (%2)").arg(name()).arg(ext_str);
}


QByteArray io::ImportExport::save(model::Document* document, const QVariantMap& setting_values, const QString& filename)
{
    QByteArray data;
    QBuffer file(&data);
    file.open(QIODevice::WriteOnly);

    QVariantMap clean_setting_values = setting_values;
    for ( const auto& setting : save_settings() )
        clean_setting_values[setting.slug] = setting.get_variant(clean_setting_values);

    if ( !save(file, filename, document, clean_setting_values) )
        return {};

    return data;
}
