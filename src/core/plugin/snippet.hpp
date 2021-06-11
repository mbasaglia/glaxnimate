#pragma once
#include "plugin.hpp"

#include "app/application.hpp"

namespace plugin {

class Snippet
{
public:
    explicit Snippet(const QString& name = {})
    {
        set_name(name);
    }

    const QString& set_name(const QString& name)
    {
        QString clean_name;
        for ( const auto& ch : name )
        {
            if ( ch.isLetterOrNumber() || ch == ' ' || ch == '_' || ch == '-' || ch == '.' || ch == '\'' || ch == '"' )
                clean_name.push_back(ch);
        }

        if ( !clean_name.isEmpty() )
        {
            if ( name_.isEmpty() )
            {
                name_ = clean_name;
            }
            else if ( name_ != clean_name && QFileInfo(filename()).exists() )
            {
                if ( QFile::rename(filename(), snippet_filename(clean_name)) )
                    name_ = clean_name;
            }
        }

        return name_;
    }

    const QString& name() const
    {
        return name_;
    }

    static QString snippet_path()
    {
        return app::Application::instance()->writable_data_path("snippets");
    }

    static QString snippet_filename(const QString& basename)
    {
        return snippet_path() + "/" + basename + ".py";
    }

    QString filename() const
    {
        return snippet_filename(name_);
    }

    QString get_source() const
    {
        QFile file(filename());
        if ( !file.open(QFile::ReadOnly|QIODevice::Text) )
            return {};
        return QString::fromUtf8(file.readAll());
    }

    bool ensure_file_exists() const
    {
        QFileInfo finfo(filename());
        if ( finfo.exists() )
            return true;

        QDir parent(snippet_path());
        if ( !parent.exists() )
        {
            parent.cdUp();
            if ( !parent.mkpath("snippets") )
                return false;
        }

        QFile file(filename());
        if ( !file.open(QFile::WriteOnly|QIODevice::Text) )
            return false;

        file.write(QApplication::tr("# Glaxnimate snippet").toUtf8());
        file.write("\n");
        file.write(QApplication::tr("# You have access to the `window` and `document` global variables and the `glaxnimate` module").toUtf8());
        file.write("\n");
        file.write(QApplication::tr("# For documentation see https://glaxnimate.mattbas.org/contributing/scripting/").toUtf8());
        file.write("\n");
        file.write("\n");
        return true;
    }

private:
    QString name_;
};

} // namespace plugin
