#include <QSplashScreen>

#include <QCommandLineParser>

#include "glaxnimate_app.hpp"
#include "app_info.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

int main(int argc, char *argv[])
{
    GlaxnimateApp app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(AppInfo::instance().description());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "File to open");
    parser.addOption({"default-ui", "If present, doen't restore the main window state"});
    parser.process(app);
    QStringList args = parser.positionalArguments();

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

    app.initialize();

    GlaxnimateWindow window(!parser.isSet("default-ui"));
    sc.finish(&window);
    window.show();

    if ( !args.empty() )
        window.document_open(args[0]);

    int ret = app.exec();

    app.finalize();

    return ret;
}
