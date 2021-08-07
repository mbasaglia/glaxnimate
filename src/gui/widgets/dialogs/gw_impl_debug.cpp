#include "glaxnimate_window_p.hpp"

#include <QShortcut>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QDomDocument>

#include "app/utils/desktop.hpp"
#include "app/debug/model.hpp"

#include "io/base.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "utils/gzip.hpp"

#include "widgets/timeline/timeline_widget.hpp"
#include "widgets/dialogs/clipboard_inspector.hpp"

#include "glaxnimate_app.hpp"


static void screenshot_widget(const QString& path, QWidget* widget)
{
    widget->show();
    QString base = widget->objectName();
    QString name = path + base.mid(base.indexOf("_")+1);
    QPixmap pic(widget->size());
    widget->render(&pic);
    name += ".png";
    pic.save(name);
}

static QString pretty_json(const QByteArray& input)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    tempf.write(QJsonDocument::fromJson(input).toJson(QJsonDocument::Indented));
    return tempf.fileName();
}

static QString pretty_xml(const QByteArray& xml)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    QDomDocument doc;
    doc.setContent(xml, false);
    tempf.write(doc.toByteArray(4));
    return tempf.fileName();
}

void GlaxnimateWindow::Private::init_debug()
{
    QMenu* menu_debug = new QMenu("Debug", parent);

    auto shortcut = new QShortcut(QKeySequence(Qt::META|Qt::Key_D), ui.canvas);
    connect(shortcut, &QShortcut::activated, parent, [menu_debug]{
            menu_debug->exec(QCursor::pos());
    });

    // Models
    QMenu* menu_print_model = new QMenu("Print Model", menu_debug);

    menu_print_model->addAction("Document Node - Full", [this]{
        app::debug::print_model(&document_node_model, {1}, false);
    });

    menu_print_model->addAction("Document Node - Layers", [this]{
        app::debug::print_model(ui.view_document_node->model(), {1}, false);
    });

    menu_print_model->addAction("Document Node - Assets", [this]{
        app::debug::print_model(&asset_model, {0}, false);
    });

    menu_print_model->addSeparator();

    menu_print_model->addAction("Properties - Single", [this]{
        app::debug::print_model(&property_model, {0}, false);
    });

    menu_print_model->addAction("Properties - Full", [this]{
        app::debug::print_model(ui.timeline_widget->raw_model(), {0}, false);
    });

    menu_print_model->addAction("Properties - Full (Filtered)", [this]{
        app::debug::print_model(ui.timeline_widget->filtered_model(), {0}, false);
    });

    menu_print_model->addAction("Properties - Full [Internals]", [this]{
        static_cast<item_models::PropertyModelFull*>(ui.timeline_widget->raw_model())->debug_tree();
    });

    menu_debug->addAction(menu_print_model->menuAction());

    // Timeline
    QMenu* menu_timeline = new QMenu("Timeline", menu_debug);
    menu_debug->addAction(menu_timeline->menuAction());
    menu_timeline->addAction("Print lines", [this]{
        ui.timeline_widget->timeline()->debug_lines();
    });
    QAction* toggle_timeline_debug = menu_timeline->addAction("Debug view");
    toggle_timeline_debug->setCheckable(true);
    connect(toggle_timeline_debug, &QAction::toggled, parent, [this](bool on){
        ui.timeline_widget->timeline()->toggle_debug(on);
        app::settings::set("internal", "debug_timeline", on);
    });
    toggle_timeline_debug->setChecked(app::settings::define("internal", "debug_timeline", false));

    // Timeline
    QMenu* menu_canvas = new QMenu("Canvas", menu_debug);
    menu_debug->addAction(menu_canvas->menuAction());
    menu_canvas->addAction("Debug Scene", [this]{ scene.debug();});

    // Screenshot
    QMenu* menu_screenshot = new QMenu("Screenshot", menu_debug);
    menu_debug->addAction(menu_screenshot->menuAction());
    menu_screenshot->addAction("Menus", [this]{
        QDir("/tmp/").mkpath("glaxnimate/menus");
        for ( auto widget : this->parent->findChildren<QMenu*>() )
            screenshot_widget("/tmp/glaxnimate/menus/", widget);
    });
    menu_screenshot->addAction("Toolbars", [this]{
        QDir("/tmp/").mkpath("glaxnimate/toolbars");
        for ( auto widget : this->parent->findChildren<QToolBar*>() )
            screenshot_widget("/tmp/glaxnimate/toolbars/", widget);
    });
    menu_screenshot->addAction("Docks", [this]{
        auto state = parent->saveState();

        QDir("/tmp/").mkpath("glaxnimate/docks");
        for ( auto widget : this->parent->findChildren<QDockWidget*>() )
        {
            widget->setFloating(true);
            screenshot_widget("/tmp/glaxnimate/docks/", widget);
        }

        parent->restoreState(state);
    });

    // Source View
    QMenu* menu_source = new QMenu("View Source", menu_debug);
    menu_debug->addAction(menu_source->menuAction());
    menu_source->addAction("Raw", [this]{
        app::desktop::open_file(current_document->io_options().filename);
    });
    menu_source->addAction("Pretty", [this]{
        QString filename = current_document->io_options().filename;
        auto fmt = current_document->io_options().format;
        if ( !fmt )
            return;

        QFile file(filename);
        if ( !file.open(QIODevice::ReadOnly) )
            return;
        QByteArray data = file.readAll();


        if ( fmt->slug() == "lottie" || fmt->slug() == "glaxnimate" )
        {
            app::desktop::open_file(pretty_json(data));
        }
        else if ( fmt->slug() == "tgs" )
        {
            QByteArray decomp;
            utils::gzip::decompress(data, decomp, {});
            app::desktop::open_file(pretty_json(decomp));
        }
        else if ( fmt->slug() == "svg" )
        {
            if ( utils::gzip::is_compressed(data) )
            {
                QByteArray decomp;
                utils::gzip::decompress(data, decomp, {});
                data = std::move(decomp);
            }

            app::desktop::open_file(pretty_xml(data));
        }
        else
        {
            app::desktop::open_file(filename);
        }
    });
    menu_source->addAction("Current", [this]{

        QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
        tempf.setAutoRemove(false);
        tempf.open();
        QJsonDocument json = io::glaxnimate::GlaxnimateFormat().to_json(current_document.get());
        tempf.write(json.toJson(QJsonDocument::Indented));
        tempf.close();
        app::desktop::open_file(tempf.fileName());
    });

    // Misc
    menu_debug->addAction("Inspect Clipboard", []{
        auto dialog = new ClipboardInspector();
        dialog->show();
        connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
    });
}
