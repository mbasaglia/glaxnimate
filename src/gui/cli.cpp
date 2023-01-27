#include "cli.hpp"

#include <QImageWriter>

#include "app/scripting/script_engine.hpp"

#include "app_info.hpp"
#include "io/io_registry.hpp"
#include "io/svg/svg_renderer.hpp"
#include "io/raster/raster_mime.hpp"

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
        {"--ipc"},
        QApplication::tr("Specify the name of the local socket/named pipe to connect to a host application."),
        app::cli::Argument::String,
        {},
        "IPC-NAME"
    });
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
        QApplication::tr("Export the input file to the given file instead of starting the GUI"),
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

    parser.add_group(QApplication::tr("Render Frame Options"));
    parser.add_argument({
        {"--render", "-r"},
        QApplication::tr("Render frames the input file to the given file instead of starting the GUI"),
        app::cli::Argument::String,
        {},
        "RENDER-FILENAME"
    });
    parser.add_argument({
        {"--render-format"},
        QApplication::tr("Specify the format for --render. If omitted it's determined based on the file name. See --render-format-list for a list of supported formats."),
        app::cli::Argument::String,
        {},
        "RENDER-FORMAT"
    });
    parser.add_argument({
        {"--frame"},
        QApplication::tr("Frame number to render, use `all` to render all frames"),
        app::cli::Argument::String,
        {"0"},
        "FRAME"
    });
    parser.add_argument({
        {"--render-format-list"},
        QApplication::tr("Shows possible values for --render-format"),
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

void log_message(const QString& message, app::log::Severity severity)
{
    app::cli::show_message(
        QApplication::tr("%1: %2")
        .arg(app::log::Logger::severity_name(severity))
        .arg(message)
    );
}


std::unique_ptr<glaxnimate::model::Document> cli_open(const app::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

    QString input_filename = args.value("file").toString();
    auto importer = io::IoRegistry::instance().from_filename(input_filename, io::ImportExport::Import);
    if ( !importer || !importer->can_open() )
    {
        app::cli::show_message(QApplication::tr("Unknown importer"), true);
        return {};
    }

    QFile input_file(input_filename);
    if ( !input_file.open(QIODevice::ReadOnly) )
    {
        app::cli::show_message(QApplication::tr("Could not open input file for reading"), true);
        return {};
    }

    auto document = std::make_unique<glaxnimate::model::Document>(input_filename);

    CliPluginExecutor script_executor(document.get());

    auto open_settings = io_settings(importer->open_settings());
    open_settings["trace"] = args.value("trace");

    QObject::connect(importer, &io::ImportExport::message, &log_message);
    if ( !importer->open(input_file, input_filename, document.get(), open_settings) )
    {
        app::cli::show_message(QApplication::tr("Error loading input file"), true);
        return {};
    }
    input_file.close();

    return document;
}

bool cli_export(const app::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

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

    auto document = cli_open(args);
    if ( !document )
        return false;

    QFile output_file(output_filename);
    if ( !output_file.open(QIODevice::WriteOnly) )
    {
        app::cli::show_message(QApplication::tr("Could not open output file for writing"), true);
        return false;
    }

    QObject::connect(exporter, &io::ImportExport::message, &log_message);
    if ( !exporter->save(output_file, output_filename, document.get(), io_settings(exporter->save_settings(document.get()))) )
    {
        app::cli::show_message(QApplication::tr("Error converting to the output format"), true);
        return false;
    }

    return true;
}

using render_funcptr = void (*)(glaxnimate::model::Document* doc, glaxnimate::model::FrameTime time, QFile& file, const char* format);

void render_frame(
    const QString& filename,
    const char* format,
    glaxnimate::model::Document* doc,
    glaxnimate::model::FrameTime time,
    render_funcptr renderer
)
{
    QFile file(filename);
    if ( !file.open(QFile::WriteOnly) )
    {
        app::cli::show_message(QApplication::tr("Could not save to %1").arg(filename), true);
        return;
    }

    renderer(doc, time, file, format);
}

void render_frame_svg(glaxnimate::model::Document* doc, glaxnimate::model::FrameTime time, QFile& file, const char*)
{
    using namespace glaxnimate;
    io::svg::SvgRenderer rend(io::svg::NotAnimated, io::svg::CssFontType::FontFace);
    doc->set_current_time(time);
    rend.write_document(doc);
    rend.write(&file, true);
}

void render_frame_img(glaxnimate::model::Document* doc, glaxnimate::model::FrameTime time, QFile& file, const char* format)
{
    QImage image = glaxnimate::io::raster::RasterMime::frame_to_image(doc->main(), time);
    if ( !image.save(&file, format) )
        app::cli::show_message(QApplication::tr("Could not save to %1").arg(file.fileName()), true);
}

bool cli_render(const app::cli::ParsedArguments& args)
{
    using namespace glaxnimate;

    if ( !args.is_defined("file") )
    {
        app::cli::show_message(QApplication::tr("You need to specify a file to render"), true);
        return false;
    }

    QString format = args.value("render-format").toString();
    QString output_filename = args.value("render").toString();
    QFileInfo finfo(output_filename);

    if ( !format.isEmpty() )
    {
        format = finfo.suffix();
        if ( !format.contains("svg") )
            format.clear();
    }

    std::string stdfmt = format.toUpper().toStdString();
    const char* cfmt = stdfmt.empty() ? nullptr : stdfmt.c_str();

    render_funcptr renderer = nullptr;

    if ( format == "svg" )
        renderer = &render_frame_svg;
    else
        renderer = &render_frame_img;

    auto document = cli_open(args);
    if ( !document )
        return false;

    auto dir = finfo.dir();
    if ( !dir.exists() )
    {
        auto name = dir.dirName();
        dir.cdUp();
        dir.mkpath(name);
        dir.cd(name);
    }

    QString frame = args.value("frame").toString();
    if ( frame == "all" || frame == "-" || frame == "*" )
    {

        float ip = document->main()->animation->first_frame.get();
        float op = document->main()->animation->last_frame.get();
        float pad = op != 0 ? std::ceil(std::log(op) / std::log(10)) : 1;

        for ( int f = ip; f < op; f += 1 )
        {
            QString frame_name = QString::number(f).rightJustified(pad, '0');
            QString file_name = dir.filePath(finfo.baseName() + frame_name + "." + finfo.completeSuffix());
            render_frame(file_name, cfmt, document.get(), f, renderer);
        }
    }
    else
    {
        render_frame(output_filename, cfmt, document.get(), frame.toDouble(), renderer);
    }

    return true;
}

} // namespace

void glaxnimate::gui::cli_main(gui::GlaxnimateApp& app, app::cli::ParsedArguments& args)
{
    app::log::Logger::instance().add_listener<app::log::ListenerStderr>();

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


    if ( args.has_flag("render-format-list") )
    {
        app::cli::show_message("svg");
        for ( const auto& fmt : QImageWriter::supportedImageFormats() )
            app::cli::show_message(QString::fromUtf8(fmt));
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

    if ( args.is_defined("render") )
    {
        app.initialize();
        if ( !cli_render(args) )
            args.return_value = 1;
        else
            args.return_value = 0;
    }
}
