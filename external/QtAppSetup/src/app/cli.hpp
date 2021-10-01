#pragma once

#include <stdexcept>
#include <optional>

#include <QVariantHash>
#include <QString>
#include <QSet>

namespace app::cli {


class ArgumentError : public std::invalid_argument
{
public:
    ArgumentError(const QString& what) : std::invalid_argument(what.toStdString()) {}

    QString message() const
    {
        return QString(what());
    }
};

struct Argument
{
    enum Type
    {
        Flag,
        String,
        Int,
        Size,
        ShowHelp,
        ShowVersion
    };

    QStringList names;
    QString description;
    Type type = String;
    QString arg_name;
    QString dest;
    int nargs = 0;
    QVariant default_value;

    Argument(
        const QStringList& names,
        const QString& description,
        Type type,
        const QVariant& default_value = {},
        const QString& arg_name = {},
        const QString& dest = {},
        int nargs = 1
    ) : names(names),
        description(description),
        type(type),
        arg_name(arg_name),
        dest(dest),
        nargs(nargs),
        default_value(default_value)
    {
        if ( this->dest.isEmpty() )
            this->dest = get_slug(names);
        if ( this->arg_name.isEmpty() )
            this->arg_name = this->dest;
    }

    Argument(
        const QStringList& names,
        const QString& description
    ) : names(names),
        description(description),
        type(Flag),
        default_value(false)
    {
        if ( this->dest.isEmpty() )
            this->dest = get_slug(names);

        // positional
        if ( names.size() == 1 && !names[0].startsWith('-') )
        {
            type = String;
            nargs = 1;
            arg_name = names[0];
        }
    }

    bool is_positional() const;


    static QString get_slug(const QStringList& names);

    QVariant arg_to_value(const QString& v, bool* ok) const;

    QVariant arg_to_value(const QString& v) const;

    QVariant args_to_value(const QStringList& args, int& index) const;

    QString help_text_name() const;

};

void show_message(const QString& msg, bool error = false);

struct ParsedArguments
{
    QVariantMap values;
    QSet<QString> defined;
    QSet<QString> flags;
    std::optional<int> return_value;

    bool has_flag(const QString& name) const
    {
        return flags.contains(name);
    }

    QVariant value(const QString& name) const
    {
        return values[name];
    }

    bool is_defined(const QString& name) const
    {
        return defined.contains(name);
    }


    void handle_error(const QString& error);
    void handle_finish(const QString& message);
};

class Parser
{
private:
    enum RefType
    {
        Option,
        Positional
    };

    struct ArgumentGroup
    {
        QString name;
        std::vector<std::pair<RefType, int>> args = {};
    };

public:
    explicit Parser(const QString& description) : description(description) {}

    app::cli::Parser& add_argument(Argument arg);
    ParsedArguments parse(const QStringList& args, int offset = 1) const;
    app::cli::Parser& add_group(const QString& name);

    const Argument* option_from_arg(const QString& arg) const;

    QString version_text() const;
    QString help_text() const;

private:
    QString wrap_text(const QString& names, int name_max, const QString& description) const;

    QString description;
    std::vector<Argument> options = {};
    std::vector<Argument> positional = {};
    std::vector<ArgumentGroup> groups = {};
};

} // namespace app::cli
