#include <QSplashScreen>
#include <QtGlobal>

#include "app/env.hpp"
#include "app/cli.hpp"
#include "app/scripting/python/python_engine.hpp"
#include "app/log/log.hpp"
#include "glaxnimate_app.hpp"
#include "app_info.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"

auto parse_cli(const QStringList& args)
{
    app::cli::Parser parser(AppInfo::instance().description());

    parser.add_group(QApplication::tr("Informational Options"));
    parser.add_argument({{"--help", "-h"}, QApplication::tr("Show this help and exit"), app::cli::Argument::ShowHelp});
    parser.add_argument({{"--version", "-v"}, QApplication::tr("Show version information and exit"), app::cli::Argument::ShowVersion});

    parser.add_group(QApplication::tr("Options"));
    parser.add_argument({{"file"}, QApplication::tr("File to open")});

    parser.add_group(QApplication::tr("GUI Options"));
    parser.add_argument({{"--default-ui"}, QApplication::tr("If present, doen't restore the main window state")});
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

    return parser.parse(args);
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    GlaxnimateApp app(argc, argv);

    AppInfo::instance().init_qapplication();

    app::scripting::python::PythonEngine::add_module_search_paths(app.data_paths("lib/"));

    auto args = parse_cli(app.arguments());

    if ( args.return_value )
        return *args.return_value;

#ifdef Q_OS_WIN
    auto pyhome = app::Environment::Variable("PYTHONHOME");
    if ( pyhome.empty() )
    {
        pyhome = app.data_file("pythonhome");
        app::log::Log("Python").log("Setting PYTHONHOME to " + pyhome.get(), app::log::Info);
        app::Environment::Variable("PYTHONPATH").push_back(app.data_file("pythonhome/lib/python"));
    }
#endif

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    GlaxnimateWindow window(!args.has_flag("default-ui"), args.has_flag("debug"));
    sc.finish(&window);
    window.show();

    if ( args.is_defined("file") )
        window.document_open(args.value("file").toString());

    if ( args.is_defined("window-size") )
        window.resize(args.value("window-size").toSize());


    if ( args.has_flag("window-id") )
        args.show_message(QString::number(window.winId(), 16), false);

    int ret = app.exec();

    app.finalize();

    return ret;
}
