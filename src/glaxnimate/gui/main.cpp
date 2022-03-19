#include <QSplashScreen>
#include <QtGlobal>

#include "app/env.hpp"
#include "app/scripting/python/python_engine.hpp"
#include "app/log/log.hpp"

#include "cli.hpp"
#include "glaxnimate/core/app_info.hpp"
#include "glaxnimate/core/io/io_registry.hpp"
#include "glaxnimate/lottie/lottie_html_format.hpp"

#include "widgets/dialogs/glaxnimate_window.hpp"

using namespace glaxnimate;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef Q_OS_WIN
    // workaround crash bug #408 in Qt/Windows
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    gui::GlaxnimateApp app(argc, argv);

    AppInfo::instance().init_qapplication();

    app::scripting::python::PythonEngine::add_module_search_paths(app.data_paths("lib/"));

    auto args = gui::parse_cli(app.arguments());

    gui::cli_main(app, args);

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

    qRegisterMetaType<app::log::Severity>();

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    bool debug = args.has_flag("debug");
    if ( debug )
        io::IoRegistry::instance().register_object(std::make_unique<io::lottie::LottieHtmlFormat>());
    gui::GlaxnimateWindow window(!args.has_flag("default-ui"), debug);
    sc.finish(&window);
    window.show();

    if (args.is_defined("ipc"))
        window.ipc_connect(args.value("ipc").toString());

    if ( args.is_defined("window-size") )
        window.resize(args.value("window-size").toSize());


    if ( args.has_flag("window-id") )
        app::cli::show_message(QString::number(window.winId(), 16), false);

    if ( args.is_defined("file") )
    {
        QVariantMap open_settings;
        open_settings["trace"] = args.value("trace");
        window.document_open_settings(args.value("file").toString(), open_settings);
    }
    else
    {
        window.show_startup_dialog();
    }

    int ret = app.exec();

    app.finalize();

    return ret;
}
