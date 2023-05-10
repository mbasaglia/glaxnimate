/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <QShortcut>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QDomDocument>
#include <QFontDatabase>

#include "app/utils/desktop.hpp"
#include "app/debug/model.hpp"

#include "io/base.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/rive/rive_format.hpp"
#include "io/lottie/lottie_format.hpp"
#include "utils/gzip.hpp"
#include "model/custom_font.hpp"

#include "widgets/timeline/timeline_widget.hpp"
#include "widgets/dialogs/clipboard_inspector.hpp"

#include "glaxnimate_app.hpp"

namespace  {

void screenshot_widget(const QString& path, QWidget* widget)
{
    widget->show();
    QString base = widget->objectName();
    QString name = path + base.mid(base.indexOf("_")+1);
    QPixmap pic(widget->size());
    widget->render(&pic);
    name += ".png";
    pic.save(name);
}

QString pretty_json(const QJsonDocument& input)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    tempf.write(input.toJson(QJsonDocument::Indented));
    return tempf.fileName();
}

QString pretty_json(const QByteArray& input)
{
    return pretty_json(QJsonDocument::fromJson(input));
}

QString pretty_xml(const QByteArray& xml)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    QDomDocument doc;
    doc.setContent(xml, false);
    tempf.write(doc.toByteArray(4));
    return tempf.fileName();
}

QString pretty_rive(const QByteArray& input)
{
    return pretty_json(glaxnimate::io::rive::RiveFormat().to_json(input));
}

void json_to_pretty_temp(const QJsonDocument& doc)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    tempf.write(doc.toJson(QJsonDocument::Indented));
    tempf.close();
    app::desktop::open_file(tempf.fileName());
}

} // namespace

void GlaxnimateWindow::Private::init_debug()
{
    QMenu* menu_debug = new QMenu("Debug", parent);

    auto shortcut = new QShortcut(QKeySequence(Qt::META|Qt::Key_D), ui.canvas);
    connect(shortcut, &QShortcut::activated, parent, [menu_debug]{
            menu_debug->exec(QCursor::pos());
    });

    // Models
    QMenu* menu_print_model = new QMenu("Print Model", menu_debug);
    menu_debug->addAction(menu_print_model->menuAction());

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

    QMenu* menu_model_signals = new QMenu("Show Model Signals", menu_debug);
    menu_debug->addAction(menu_model_signals->menuAction());
    menu_model_signals->addAction("Document Node - Full", [this]{
        app::debug::connect_debug(&document_node_model, "Document Node - Full");
    });
    menu_model_signals->addAction("Document Node - Layers", [this]{
        app::debug::connect_debug(ui.view_document_node->model(), "Document Node - Layers");
    });

    menu_debug->addAction("Current index", [this]{

        auto layers_index = ui.view_document_node->currentIndex();
        qDebug() << "Layers" << layers_index << ui.view_document_node->current_node() << ui.view_document_node->node(layers_index);


        qDebug() << "Timeline" << ui.timeline_widget->current_index_raw() << ui.timeline_widget->current_index_filtered() << ui.timeline_widget->current_node();
    });

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
        else if ( fmt->slug() == "rive" )
        {
            app::desktop::open_file(pretty_rive(data));
        }
        else
        {
            app::desktop::open_file(filename);
        }
    });
    menu_source->addAction("Current (Rawr)", [this]{
        json_to_pretty_temp(io::glaxnimate::GlaxnimateFormat().to_json(current_document.get()));
    });
    menu_source->addAction("Current (Lottie)", [this]{
        json_to_pretty_temp(
            QJsonDocument(io::lottie::LottieFormat().to_json(comp).toJsonObject())
        );
    });
    menu_source->addAction("Current (RIVE)", [this]{
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        io::rive::RiveFormat().save(buffer, "", comp, {});
        json_to_pretty_temp(io::rive::RiveFormat().to_json(buffer.data()));
    });

    // Misc
    menu_debug->addAction("Inspect Clipboard", []{
        auto dialog = new ClipboardInspector();
        dialog->show();
        connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
    });
    menu_debug->addAction("Fonts", []{
        qDebug() << "---- Fonts ----";
        QFontDatabase db;
        for ( const auto& family : db.families() )
            qDebug() << family;

        qDebug() << "---- Custom ----";
        for ( const auto& font : model::CustomFontDatabase::instance().fonts() )
            qDebug() << font.family() << ":" << font.style_name();

        qDebug() << "---- Aliases ----";
        for ( const auto& p : model::CustomFontDatabase::instance().aliases() )
        {
            auto db = qDebug();
            db << p.first << "->";
            for ( const auto& n : p.second )
                db << n;
        }
        qDebug() << "----";
    });

}
