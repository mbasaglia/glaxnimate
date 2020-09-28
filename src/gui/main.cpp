#include <QSplashScreen>

// #include <QCommandLineParser>

#include "app/cli.hpp"
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

    return parser.parse(args);
}

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    app.init_info();

    auto args = parse_cli(app.arguments());

    if ( args.return_value )
        return *args.return_value;

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    GlaxnimateWindow window(!args.has_flag("default-ui"));
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
