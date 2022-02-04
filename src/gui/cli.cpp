#include "cli.hpp"

#include "app/scripting/script_engine.hpp"

#include "app_info.hpp"
#include "io/io_registry.hpp"

#include "plugin/executor.hpp"
#include "plugin/plugin.hpp"

app::cli::ParsedArguments glaxnimate::gui::parse_cli(const QStringList& args)
{
    app::cli::Parser parser(AppInfo::instance().description());

    parser.add_group(QApplication::tr("Informational Options"));
    parser.add_argument({{"--help", "-h"}, QApplication::tr("Show this help and exit"), app::cli::Argument::ShowHelp});
    parser.add_argument({{"--version", "-v"}, QApplication::tr("Show version information and exit"), app::cli::Argument::ShowVersion});

    parser.add_group(QApplication::tr("Options"));
    parser.add_argument({{"file"}, QApplication::tr("File to open")});
    parser.add_argument({{"--trace"}, QApplication::tr("When opening image files, trace them instead of embedding")});

    parser.add_group(QApplication::tr("GUI Options"));
    parser.add_argument({{"--default-ui"}, QApplication::tr("If present, doesn't restore the main window state")});
    parser.add_argument({
        {"--window-size"},
        QApplication::tr("Use a specific size for the main window"),
        app::cli::Argument::Size,
        {},
        "WIDTHxHEIGHT"
    });

    parser.add_argument({
        {"--window-id"},
        QApplication::tr("Print the window id"),
    });

    parser.add_argument({{"--debug"}, QApplication::tr("Enables the debug menu")});


    parser.add_group(QApplication::tr("Export Options"));
    parser.add_argument({
        {"--export", "-o"},
        QApplication::tr("If present, instead of starting the GUI export the input file to the given"),
        app::cli::Argument::String,
        {},
        "EXPORT-FILENAME"
    });
    parser.add_argument({
        {"--export-format", "-f"},
        QApplication::tr("Specify the format for --export. If omitted it's determined based on the file name. See --export-format-list for a list of supported formats."),
        app::cli::Argument::String,
        {},
        "EXPORT-FORMAT"
    });
    parser.add_argument({
        {"--export-format-list"},
        QApplication::tr("Shows possible values for --export-format"),
        app::cli::Argument::Flag
    });

    return parser.parse(args);
}


namespace  {

/// \todo A lot of code copied from the console, maybe could be moved to Executor
class CliPluginExecutor : public glaxnimate::plugin::Executor
{
public:
    CliPluginExecutor(glaxnimate::model::Document* document)
    {
        globals["document"] = QVariant::fromValue(document);
        globals["window"] = {};

        for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
        {
            auto ctx = engine->create_context();

            if ( !ctx )
                continue;

            QObject::connect(ctx.get(), &app::scripting::ScriptExecutionContext::stdout_line, [this](const QString& s){ console_stdout(s);});
            QObject::connect(ctx.get(), &app::scripting::ScriptExecutionContext::stderr_line, [this](const QString& s){ console_stderr(s);});

            try {
                ctx->app_module("glaxnimate");
                ctx->app_module("glaxnimate_gui");
                for ( const auto& p : globals )
                    ctx->expose(p.first, p.second);
            } catch ( const app::scripting::ScriptError& err ) {
                console_stderr(err.message());
            }

            script_contexts.push_back(std::move(ctx));
        }

        glaxnimate::plugin::PluginRegistry::instance().set_executor(this);
    }

    bool execute(const glaxnimate::plugin::Plugin& plugin, const glaxnimate::plugin::PluginScript& script, const QVariantList& args) override
    {
        for ( const auto& ctx : script_contexts )
        {
            if ( ctx->engine() == plugin.data().engine )
            {
                bool ok = false;
                try {
                    ok = ctx->run_from_module(plugin.data().dir, script.module, script.function, args);
                    if ( !ok )
                        console_stderr(QApplication::tr("Could not run the plugin"));
                } catch ( const app::scripting::ScriptError& err ) {
                    console_stderr(err.message());
                    ok = false;
                }
                return ok;
            }
        }

        console_stderr(QApplication::tr("Could not find an interpreter"));
        return false;
    }

    QVariant get_global(const QString& name) override
    {
        auto it = globals.find(name);
        if ( it != globals.end() )
            return it->second;
        return {};
    }

    void console_stderr(const QString& line)
    {
        app::cli::show_message(line, true);
    }

    void console_stdout(const QString& line)
    {
        app::cli::show_message(line, false);
    }

private:
    std::vector<app::scripting::ScriptContext> script_contexts;
    std::map<QString, QVariant> globals;
};


QVariantMap io_settings(std::unique_ptr<app::settings::SettingsGroup> group)
{
    QVariantMap vals;
    if ( group )
    {
        for ( const auto& setting : *group )
            vals[setting.slug] = setting.default_value;
    }
    return vals;
}

} // namespace

bool glaxnimate::gui::cli_export(const app::cli::ParsedArguments& args)
{
    if ( !args.is_defined("file") )
    {
        app::cli::show_message(QApplication::tr("You need to specify a file to export"), true);
        return false;
    }


    io::ImportExport* exporter = nullptr;
    QString format = args.value("export-format").toString();
    QString output_filename = args.value("export").toString();

    if ( !format.isEmpty() )
        exporter = io::IoRegistry::instance().from_slug(format);
    else
        exporter = io::IoRegistry::instance().from_filename(output_filename, io::ImportExport::Export);

    if ( !exporter || !exporter->can_save() )
    {
        app::cli::show_message(QApplication::tr("Unknown exporter. use --export-format-list for a list of available formats"), true);
        return false;
    }


    QString input_filename = args.value("file").toString();
    auto importer = io::IoRegistry::instance().from_filename(input_filename, io::ImportExport::Import);
    if ( !importer || !importer->can_open() )
    {
        app::cli::show_message(QApplication::tr("Unknown importer"), true);
        return false;
    }

    QFile input_file(input_filename);
    if ( !input_file.open(QIODevice::ReadOnly) )
    {
        app::cli::show_message(QApplication::tr("Could not open input file for reading"), true);
        return false;
    }

    model::Document document(input_filename);

    CliPluginExecutor script_executor(&document);


    auto open_settings = io_settings(importer->open_settings());
    open_settings["trace"] = args.value("trace");

    if ( !importer->open(input_file, input_filename, &document, open_settings) )
    {
        app::cli::show_message(QApplication::tr("Error loading input file"), true);
        return false;
    }
    input_file.close();

    QFile output_file(output_filename);
    if ( !output_file.open(QIODevice::WriteOnly) )
    {
        app::cli::show_message(QApplication::tr("Could not open output file for writing"), true);
        return false;
    }

    if ( !exporter->save(output_file, output_filename, &document, io_settings(exporter->save_settings(&document))) )
    {
        app::cli::show_message(QApplication::tr("Error converting to the output format"), true);
        return false;
    }

    return true;
}

void glaxnimate::gui::cli_main(gui::GlaxnimateApp& app, app::cli::ParsedArguments& args)
{
    if ( args.has_flag("export-format-list") )
    {
        app.initialize();
        int max_name_len = 0;
        std::vector<std::pair<QString, QString>> table;
        for ( const auto& exporter : io::IoRegistry::instance().exporters() )
        {
            table.emplace_back(exporter->slug(), exporter->name());
            max_name_len = qMax<int>(table.back().first.size(), max_name_len);
        }
        for ( const auto& entry : table )
            app::cli::show_message(entry.first + QString(max_name_len - entry.first.size(), ' ') + " : " + entry.second, false);
        args.return_value = 0;
        return;
    }

    if ( args.is_defined("export") )
    {
        app.initialize();
        if ( !cli_export(args) )
            args.return_value = 1;
        else
            args.return_value = 0;
    }
}


