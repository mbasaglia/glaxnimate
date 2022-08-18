#include "cli.hpp"

#include <cstdio>

#include <QVector>
#include <QSize>
#include <QApplication>


QString app::cli::Argument::get_slug(const QStringList& names)
{
    if ( names.empty() )
        return {};

    QString match;
    for ( const auto& name: names )
    {
        if ( name.size() > match.size() )
            match = name;
    }

    for ( int i = 0; i < match.size(); i++ )
    {
        if ( match[i] != '-' )
            return match.mid(i);
    }

    return {};
}

QVariant app::cli::Argument::arg_to_value(const QString& v, bool* ok) const
{
    switch ( type )
    {
        case String:
            *ok = true;
            return v;
        case Int:
            return v.toInt(ok);
        case Size:
        {
            if ( !v.contains('x') )
            {
                *ok = false;
                return {};
            }

            auto vec = QStringView{v}.split('x');
            if ( vec.size() != 2 )
            {
                *ok = false;
                return {};
            }

            *ok = true;
            int x = vec[0].toInt(ok);
            if ( !ok )
                return {};

            int y = vec[1].toInt(ok);
            if ( !ok )
                return {};

            return QSize(x, y);
        }
        case Flag:
        case ShowHelp:
        case ShowVersion:
            *ok = false;
            return {};
    }

    *ok = false;
    return {};
}

QVariant app::cli::Argument::arg_to_value(const QString& arg) const
{
    bool ok = false;
    QVariant v = arg_to_value(arg, &ok);
    if ( !ok )
        throw ArgumentError(
            QApplication::tr("%2 is not a valid value for %1")
            .arg(names[0]).arg(arg)
        );
    return v;
}


QVariant app::cli::Argument::args_to_value(const QStringList& args, int& index) const
{
    if ( type == Flag )
        return true;

    if ( args.size() - index < nargs )
        throw ArgumentError(
            QApplication::tr("Not enough arguments for %1: needs %2, has %3")
            .arg(names[0]).arg(nargs).arg(args.size() - index)
        );

    if ( nargs == 1 )
        return arg_to_value(args[index++]);

    QVariantList vals;
    for ( int i = 0; i < nargs; i++ )
        vals.push_back(arg_to_value(args[index++]));
    return vals;
}


QString app::cli::Argument::help_text_name() const
{
    QString option_names;
    for ( const auto& name : names )
        option_names += name + ", ";
    if ( !names.isEmpty() )
        option_names.chop(2);

    if ( !arg_name.isEmpty() )
        option_names += " <" + arg_name + ">";

    if ( nargs > 1 )
        option_names += "...";

    return option_names;
}


bool app::cli::Argument::is_positional() const
{
    return names.size() == 1 && !names[0].startsWith('-') && nargs > 0;
}


app::cli::Parser& app::cli::Parser::add_argument(Argument arg)
{
    if ( groups.empty() )
        groups.push_back({QApplication::tr("Options")});

    if ( arg.is_positional() )
    {
        groups.back().args.emplace_back(Positional, positional.size());
        positional.emplace_back(std::move(arg));
    }
    else
    {
        groups.back().args.emplace_back(Option, options.size());
        options.emplace_back(std::move(arg));
    }

    return *this;
}

app::cli::Parser & app::cli::Parser::add_group(const QString& name)
{
    groups.push_back({name});
    return *this;
}

const app::cli::Argument * app::cli::Parser::option_from_arg(const QString& arg) const
{
    for ( const auto& option : options )
        if ( option.names.contains(arg) )
            return &option;
    return nullptr;
}

void app::cli::ParsedArguments::handle_error(const QString& error)
{
    show_message(error, true);
    return_value = 1;
}

void app::cli::ParsedArguments::handle_finish(const QString& message)
{
    show_message(message, false);
    return_value = 0;
}


app::cli::ParsedArguments app::cli::Parser::parse(const QStringList& args, int offset) const
{
    int next_positional = 0;

    ParsedArguments parsed;
    for ( const auto& option : options )
        parsed.values[option.dest] = option.default_value;

    for ( int index = offset; index < args.size(); )
    {
        if ( args[index].startsWith('-') )
        {
            if ( auto opt = option_from_arg(args[index]) )
            {
                if ( opt->type == Argument::ShowHelp )
                {
                    parsed.handle_finish(help_text());
                    break;
                }
                else if ( opt->type == Argument::ShowVersion )
                {
                    parsed.handle_finish(version_text());
                    break;
                }

                index++;
                QVariant val;
                try {
                    val = opt->args_to_value(args, index);
                } catch ( const ArgumentError& err ) {
                    parsed.handle_error(err.message());
                    break;
                }
                parsed.values[opt->dest] = val;
                parsed.defined.insert(opt->dest);
                if ( opt->type == Argument::Flag && val.toBool() )
                    parsed.flags.insert(opt->dest);
                continue;
            }

            parsed.handle_error(QApplication::tr("Unknown argument %1").arg(args[index]));
            index++;
            break;
        }

        if ( next_positional >= int(positional.size()) )
        {
            parsed.handle_error(QApplication::tr("Too many arguments"));
            break;
        }

        auto arg = &positional[next_positional];
        parsed.defined.insert(arg->dest);
        try {
            parsed.values[arg->dest] = arg->args_to_value(args, index);
        } catch ( const ArgumentError& err ) {
            parsed.handle_error(err.message());
            break;
        }
        next_positional++;
    }

    return parsed;
}

QString app::cli::Parser::version_text() const
{
    return QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion() + "\n";
}

void app::cli::show_message(const QString& msg, bool error)
{
    std::fputs(qUtf8Printable(msg + '\n'), error ? stderr : stdout);
}


QString app::cli::Parser::help_text() const
{
    QString usage = QCoreApplication::instance()->arguments().constFirst();

    if ( !options.empty() )
        usage += QApplication::tr(" [options]");


    int longest_name = 0;
    QStringList opt_names;
    QStringList pos_names;

    for ( const auto& opt : options )
    {
        QString name = opt.help_text_name();
        if ( longest_name < name.size() )
            longest_name = name.size();
        opt_names.append(name);
    }

    for ( const auto& pos : positional )
    {
        usage += " " + pos.arg_name;
        QString name = pos.help_text_name();
        if ( longest_name < name.size() )
            longest_name = name.size();
        pos_names.append(name);
    }

    QString text;
    text += QApplication::tr("Usage: %1").arg(usage);
    text += '\n';
    text += '\n';
    text += description;
    text += '\n';

    for ( const auto& grp : groups )
    {
        text += '\n';
        text += grp.name;
        text += ":\n";
        for ( const auto& p : grp.args )
        {
            text += wrap_text(
                (p.first == Positional ? pos_names : opt_names)[p.second],
                longest_name,
                (p.first == Positional ? positional : options)[p.second].description
            );
            text += '\n';
        }
    }

    return text;
}

QString app::cli::Parser::wrap_text(const QString& names, int name_max, const QString& description) const
{
    const QLatin1String indentation("  ");

    // In case the list of option names is very long, wrap it as well
    int nameIndex = 0;
    auto nextNameSection = [&]() {
        QString section = names.mid(nameIndex, name_max);
        nameIndex += section.size();
        return section;
    };

    QString text;
    int lineStart = 0;
    int lastBreakable = -1;
    const int max = 79 - (indentation.size() + name_max + 1);
    int x = 0;
    const int len = description.length();

    for (int i = 0; i < len; ++i)
    {
        ++x;
        const QChar c = description.at(i);
        if (c.isSpace())
            lastBreakable = i;

        int breakAt = -1;
        int nextLineStart = -1;
        if (x > max && lastBreakable != -1) {
            // time to break and we know where
            breakAt = lastBreakable;
            nextLineStart = lastBreakable + 1;
        } else if ((x > max - 1 && lastBreakable == -1) || i == len - 1) {
            // time to break but found nowhere [-> break here], or end of last line
            breakAt = i + 1;
            nextLineStart = breakAt;
        } else if (c == '\n') {
            // forced break
            breakAt = i;
            nextLineStart = i + 1;
        }

        if (breakAt != -1) {
            const int numChars = breakAt - lineStart;
            text += indentation + nextNameSection().leftJustified(name_max) + QLatin1Char(' ');
            text += QStringView{description}.mid(lineStart, numChars);
            text += '\n';
            x = 0;
            lastBreakable = -1;
            lineStart = nextLineStart;
            if (lineStart < len && description.at(lineStart).isSpace())
                ++lineStart; // don't start a line with a space
            i = lineStart;
        }
    }

    while (nameIndex < names.size()) {
        text += indentation + nextNameSection() + '\n';
    }

    return text;
}


