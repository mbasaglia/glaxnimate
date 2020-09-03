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

