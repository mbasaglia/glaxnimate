#include "glaxnimate_window_p.hpp"

#include <QTemporaryFile>
#include <QDesktopServices>
#include <QFileDialog>
#include <QImageWriter>
#include <QDropEvent>
#include <QtConcurrent>
#include <QEventLoop>


#include "io/lottie/lottie_html_format.hpp"
#include "io/svg/svg_renderer.hpp"
#include "io/svg/svg_html_format.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/raster/raster_mime.hpp"
#include "io/lottie/tgs_format.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/undo_macro_guard.hpp"

#include "tools/base.hpp"

#include "glaxnimate_app.hpp"
#include "app_info.hpp"
#include "widgets/dialogs/import_export_dialog.hpp"
#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"


static void process_events(const QFuture<bool>& promise)
{
    while ( !promise.isFinished() )
    {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents|QEventLoop::WaitForMoreEvents, 10);
    }
}

void GlaxnimateWindow::Private::setup_document(const QString& filename)
{
    if ( !close_document() )
        return;

    current_document = std::make_unique<model::Document>(filename);

    do_setup_document();
}

void GlaxnimateWindow::Private::setup_document_ptr(std::unique_ptr<model::Document> doc)
{
    if ( !close_document() )
        return;

    current_document = std::move(doc);

    do_setup_document();

    QDir path = app::settings::get<QString>("open_save", "path");
    auto opts = current_document->io_options();
    opts.path = path;
    current_document->set_io_options(opts);

    view_fit();
    if ( !current_document->main()->shapes.empty() )
        ui.view_document_node->set_current_node(current_document->main()->shapes[0]);

    ui.timeline_widget->reset_view();
    ui.play_controls->set_range(current_document->main()->animation->first_frame.get(), current_document->main()->animation->last_frame.get());

}

void GlaxnimateWindow::Private::do_setup_document()
{
    current_document_has_file = false;

    // Composition
    comp = current_document->main();
    connect(current_document->assets()->precompositions.get(), &model::PrecompositionList::docnode_child_remove_begin, parent, [this](int index){on_remove_precomp(index);});
    connect(current_document->assets()->precompositions.get(), &model::PrecompositionList::precomp_added, parent, [this](model::Precomposition* node, int row){setup_composition(node, row+1);});
    ui.menu_new_comp_layer->setEnabled(false);
    setup_composition(current_document->main());

    // Undo Redo
    QObject::connect(ui.action_redo, &QAction::triggered, &current_document->undo_stack(), &QUndoStack::redo);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::canRedoChanged, ui.action_redo, &QAction::setEnabled);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::redoTextChanged, ui.action_redo, [this](const QString& s){
        ui.action_redo->setText(redo_text.arg(s));
    });
    ui.action_redo->setEnabled(current_document->undo_stack().canRedo());
    ui.action_redo->setText(redo_text.arg(current_document->undo_stack().redoText()));

    QObject::connect(ui.action_undo, &QAction::triggered, &current_document->undo_stack(), &QUndoStack::undo);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::canUndoChanged, ui.action_undo, &QAction::setEnabled);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::undoTextChanged, ui.action_undo, [this](const QString& s){
        ui.action_undo->setText(undo_text.arg(s));
    });
    ui.action_undo->setEnabled(current_document->undo_stack().canUndo());
    ui.action_undo->setText(redo_text.arg(current_document->undo_stack().undoText()));

    // Views
    document_node_model.set_document(current_document.get());
    ui.view_document_node->set_composition(comp);
    ui.timeline_widget->set_document(current_document.get());
    ui.timeline_widget->set_composition(comp);
    ui.view_assets->setRootIndex(asset_model.mapFromSource(document_node_model.node_index(current_document->assets()).siblingAtColumn(1)));

    property_model.set_document(current_document.get());
    property_model.set_object(current_document->main());
    ui.tab_bar->set_document(current_document.get());

    scene.set_document(current_document.get());

    ui.view_undo->setStack(&current_document->undo_stack());

    ui.document_swatch_widget->set_document(current_document.get());
    ui.widget_gradients->set_document(current_document.get());

    // Scripting
    ui.console->clear_contexts();
    ui.console->set_global("document", QVariant::fromValue(current_document.get()));

    // Title
    QObject::connect(current_document.get(), &model::Document::filename_changed, parent, &GlaxnimateWindow::refresh_title);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::cleanChanged, parent, &GlaxnimateWindow::refresh_title);
    refresh_title();

    // Playback
    ui.play_controls->set_record_enabled(false);
    QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &FrameControlsWidget::set_min);
    QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &FrameControlsWidget::set_max);;
    QObject::connect(current_document->main(), &model::MainComposition::fps_changed, ui.play_controls, &FrameControlsWidget::set_fps);
    QObject::connect(ui.play_controls, &FrameControlsWidget::frame_selected, current_document.get(), &model::Document::set_current_time);
    QObject::connect(current_document.get(), &model::Document::current_time_changed, ui.play_controls, &FrameControlsWidget::set_frame);
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, ui.play_controls, &FrameControlsWidget::set_record_enabled);
    QObject::connect(ui.play_controls, &FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);

    widget_recording->setVisible(false);
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, widget_recording, &QWidget::setVisible);

    // Export
    export_options = {};
    ui.action_export->setText(tr("Export..."));
}

void GlaxnimateWindow::Private::setup_document_new(const QString& filename)
{
    setup_document(filename);

    current_document->main()->name.set(current_document->main()->type_name_human());
    current_document->main()->width.set(app::settings::get<int>("defaults", "width"));
    current_document->main()->height.set(app::settings::get<int>("defaults", "height"));
    current_document->main()->fps.set(app::settings::get<float>("defaults", "fps"));
    float duration = app::settings::get<float>("defaults", "duration");
    int out_point = current_document->main()->fps.get() * duration;
    current_document->main()->animation->last_frame.set(out_point);


    auto layer = std::make_unique<model::Layer>(current_document.get());
    layer->animation->last_frame.set(out_point);
    layer->name.set(layer->type_name_human());
    QPointF pos(
        current_document->main()->width.get() / 2.0,
        current_document->main()->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    model::ShapeElement* ptr = layer.get();
    current_document->main()->shapes.insert(std::move(layer), 0);

    QDir path = app::settings::get<QString>("open_save", "path");
    auto opts = current_document->io_options();
    opts.path = path;
    current_document->set_io_options(opts);

    ui.view_document_node->set_current_node(ptr);
    ui.play_controls->set_range(0, out_point);
    view_fit();

    ui.timeline_widget->reset_view();
}

bool GlaxnimateWindow::Private::setup_document_open(const io::Options& options)
{
    setup_document(options.filename);

    current_document_has_file = true;
    dialog_import_status->reset(options.format, options.filename);


    // Note to use a separate thread here, we need to make use model functions are thread safe as
    // we emit signals when adding object and access them from the main thread
    QFile file(options.filename);
    bool ok = options.format->open(file, options.filename, current_document.get(), options.settings);

    app::settings::set<QString>("open_save", "path", options.path.absolutePath());

    if ( ok && !autosave_load )
        most_recent_file(options.filename);

    view_fit();
    if ( !current_document->main()->shapes.empty() )
        ui.view_document_node->set_current_node(current_document->main()->shapes[0]);

    current_document->set_io_options(options);
    auto first_frame = current_document->main()->animation->first_frame.get();
    ui.play_controls->set_range(first_frame, current_document->main()->animation->last_frame.get());
    current_document->set_current_time(first_frame);

    if ( !autosave_load && QFileInfo(backup_name()).exists() )
    {
        WindowMessageWidget::Message msg{
            tr("Looks like this file is being edited by another Glaxnimate instance or it was being edited when Glaxnimate crashed."),
            app::log::Info
        };

        msg.add_action(
            QIcon::fromTheme("document-close"),
            tr("Close Document"),
            parent,
            &GlaxnimateWindow::document_new
        );

        msg.add_action(
            QIcon::fromTheme("document-revert"),
            tr("Load Backup"),
            parent,
            [this, uuid=current_document->main()->uuid.get()]{ load_backup(uuid); }
        );

        ui.message_widget->queue_message(std::move(msg));
    }

    export_options = options;
    export_options.filename = "";

    ui.timeline_widget->reset_view();

    return ok;
}


void GlaxnimateWindow::Private::refresh_title()
{
    QString title = current_document->filename();
    if ( !current_document->undo_stack().isClean() )
        title += " *";
    parent->setWindowTitle(title);
}

bool GlaxnimateWindow::Private::close_document()
{
    if ( current_document )
    {
        if ( !autosave_load )
            QDir().remove(backup_name());

        if ( !current_document->undo_stack().isClean() )
        {
            QMessageBox warning(parent);
            warning.setWindowTitle(QObject::tr("Closing Animation"));
            warning.setText(QObject::tr("The animation has unsaved changes.\nDo you want to save your changes?"));
            warning.setInformativeText(current_document->filename());
            warning.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            warning.setDefaultButton(QMessageBox::Save);
            warning.setIcon(QMessageBox::Warning);
            int result = warning.exec();

            if ( result == QMessageBox::Save )
            {
                if ( !save_document(false, false) )
                    return false;
            }
            else if ( result == QMessageBox::Cancel )
            {
                return false;
            }

            // Prevent signals on the destructor
            current_document->undo_stack().clear();
        }
    }

    if ( active_tool )
        active_tool->close_document_event({ui.canvas, &scene, parent});

    comp = nullptr;
    ui.stroke_style_widget->set_shape(nullptr);
    ui.fill_style_widget->set_shape(nullptr);
    document_node_model.clear_document();
    property_model.clear_document();
    scene.clear_document();
    ui.timeline_widget->clear_document();
    ui.view_undo->setStack(nullptr);
    ui.document_swatch_widget->set_document(nullptr);
    ui.widget_gradients->set_document(nullptr);
    ui.view_document_node->set_composition(nullptr);
    ui.tab_bar->set_document(nullptr);

    comp_selections.clear();
    ui.menu_new_comp_layer->clear();

    return true;
}

bool GlaxnimateWindow::Private::save_document(bool force_dialog, bool export_opts)
{
    io::Options opts = export_opts ? export_options : current_document->io_options();

    if ( !opts.format || !opts.format->can_save() || !current_document_has_file || opts.filename.isEmpty() )
        force_dialog = true;

    if ( force_dialog )
    {
        ImportExportDialog dialog(opts, parent);

        if ( !dialog.export_dialog() )
            return false;

        opts = dialog.io_options();
    }

    dialog_export_status->reset(opts.format, opts.filename);

    auto promise = QtConcurrent::run(
        [opts, current_document=current_document.get()]{
            QFile file(opts.filename);
            return opts.format->save(file, opts.filename, current_document, opts.settings);
        });

    process_events(promise);

    if ( !promise.result() )
        return false;

    if ( export_opts )
    {
        export_options = opts;
        ui.action_export->setText(tr("Export to %1").arg(QFileInfo(opts.filename).fileName()));
    }
    else
    {
        if ( opts.format->can_open() )
            most_recent_file(opts.filename);
        current_document->undo_stack().setClean();
        current_document_has_file = true;
        app::settings::set<QString>("open_save", "path", opts.path.absolutePath());
        current_document->set_io_options(opts);
    }

    return true;
}

void GlaxnimateWindow::Private::document_open()
{
    io::Options options = current_document->io_options();

    ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
    if ( dialog.import_dialog() )
        setup_document_open(dialog.io_options());
}


void GlaxnimateWindow::Private::document_open_from_filename(const QString& filename)
{
    QFileInfo finfo(filename);
    if ( finfo.isFile() )
    {
        io::Options opts;
        opts.format = io::IoRegistry::instance().from_filename(filename);
        opts.path = finfo.dir();
        opts.filename = filename;

        if ( opts.format && opts.format->can_open() )
        {
            ImportExportDialog dialog(opts, ui.centralwidget->parentWidget());
            if ( dialog.options_dialog(opts.format->open_settings()) )
                setup_document_open(dialog.io_options());

            return;
        }
        else
        {
            show_warning(tr("Open File"), tr("No importer found for %1").arg(filename));
        }
    }
    else
    {
        show_warning(tr("Open File"), tr("The file might have been moved or deleted\n%1").arg(filename));
    }


    recent_files.removeAll(filename);
    reload_recent_menu();
}


void GlaxnimateWindow::Private::document_reload()
{
    if ( !current_document_has_file )
    {
        status_message(tr("No file to reload from"));
        return;
    }

    auto options = current_document->io_options();
    setup_document_open(options);
}


void GlaxnimateWindow::Private::preview(io::ImportExport& exporter, const QVariantMap& options)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX." + exporter.extensions()[0]);
    tempf.setAutoRemove(false);
    bool ok = tempf.open() && exporter.save(
        tempf, tempf.fileName(), current_document.get(), options
    );

    if ( !ok )
    {
        show_warning(tr("Web Preview"), tr("Could not create file"));
        return;
    }

    tempf.close();
    if ( !QDesktopServices::openUrl(QUrl::fromLocalFile(tempf.fileName())) )
    {
        show_warning(tr("Web Preview"), tr("Could not open browser"));
    }
}

void GlaxnimateWindow::Private::preview_lottie(const QString& renderer)
{
    io::lottie::LottieHtmlFormat fmt;
    preview(fmt, {{"renderer", renderer}});
}

void GlaxnimateWindow::Private::preview_svg()
{
    io::svg::SvgHtmlFormat fmt;
    preview(fmt, {});
}

void GlaxnimateWindow::Private::save_frame_bmp()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, tr("Save Frame Image"));
    fd.setDirectory(app::settings::get<QString>("open_save", "render_path"));
    fd.setDefaultSuffix("png");
    fd.selectFile(tr("Frame%1.png").arg(frame));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);

    QString formats;
    for ( const auto& fmt : QImageWriter::supportedImageFormats() )
        formats += QString("*.%1 ").arg(QString::fromUtf8(fmt));
    fd.setNameFilter(tr("Image files (%1)").arg(formats));

    if ( fd.exec() == QDialog::Rejected )
        return;

    app::settings::set("open_save", "render_path", fd.directory().path());

    QImage image = io::raster::RasterMime().to_image({current_document->main()});
    if ( !image.save(fd.selectedFiles()[0]) )
        show_warning(tr("Render Frame"), tr("Could not save image"));
}


void GlaxnimateWindow::Private::save_frame_svg()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, tr("Save Frame Image"));
    fd.setDirectory(app::settings::get<QString>("open_save", "render_path"));
    fd.setDefaultSuffix("svg");
    fd.selectFile(tr("Frame%1.svg").arg(frame));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setNameFilter(tr("Scalable Vector Graphics (*.svg)"));

    if ( fd.exec() == QDialog::Rejected )
        return;

    app::settings::set("open_save", "render_path", fd.directory().path());

    QFile file(fd.selectedFiles()[0]);
    if ( !file.open(QFile::WriteOnly) )
    {
        show_warning(tr("Render Frame"), tr("Could not save image"));
        return;
    }

    io::svg::SvgRenderer rend(io::svg::NotAnimated);
    rend.write_document(current_document.get());
    rend.write(&file, true);
}

void GlaxnimateWindow::Private::validate_tgs()
{
    IoStatusDialog dialog(QIcon::fromTheme("telegram"), "", false, parent);
    io::lottie::TgsFormat fmt;
    dialog.reset(&fmt, tr("Validate Telegram Sticker"));
    fmt.validate(current_document.get());
    dialog.show_errors(tr("No issues found"), tr("Some issues detected"));
    dialog.exec();
}

void GlaxnimateWindow::Private::autosave_timer_start(int mins)
{
    if ( mins == -1 )
        mins = app::settings::get<int>("open_save", "backup_frequency");
    autosave_timer_mins = mins;
    if ( autosave_timer_mins )
        autosave_timer = parent->startTimer(autosave_timer_mins * 1000 * 60);
}

void GlaxnimateWindow::Private::autosave_timer_tick()
{
    if ( current_document && !current_document->undo_stack().isClean() && !autosave_load )
    {
        QFile file(backup_name());
        file.open(QIODevice::WriteOnly);
        io::glaxnimate::GlaxnimateFormat().save(file, file.fileName(), current_document.get(), {});
    }

}

void GlaxnimateWindow::Private::autosave_timer_load_settings()
{
    int mins = app::settings::get<int>("open_save", "backup_frequency");
    if ( mins != autosave_timer_mins )
    {
        if ( autosave_timer )
            parent->killTimer(autosave_timer);
        autosave_timer = 0;
        autosave_timer_start(mins);
    }
}

QString GlaxnimateWindow::Private::backup_name()
{
    return backup_name(current_document->main()->uuid.get());
}

QString GlaxnimateWindow::Private::backup_name(const QUuid& id)
{
    return GlaxnimateApp::instance()->backup_path(id.toString(QUuid::Id128) + ".bak.rawr");
}

void GlaxnimateWindow::Private::load_backup(const QUuid& id)
{
    if ( id != current_document->main()->uuid.get() )
    {
        show_warning(tr("Backup"), tr("Cannot load backup of a closed file"));
        return;
    }

    auto io_options_old = current_document->io_options();

    io::Options io_options_bak {
        io::glaxnimate::GlaxnimateFormat::instance(),
        GlaxnimateApp::instance()->backup_path(),
        backup_name(id),
        {}
    };

    autosave_load = true;
    setup_document_open(io_options_bak);
    current_document->set_io_options(io_options_old);
    autosave_load = false;
}


QString GlaxnimateWindow::Private::drop_event_data(QDropEvent* event)
{
    const QMimeData* data = event->mimeData();

    if ( !data->hasUrls() )
       return {};

    for ( const auto& url : data->urls() )
    {
        if ( url.isLocalFile() )
        {
            QString filename = url.toLocalFile();
            QString extension = QFileInfo(filename).completeSuffix();
            if ( auto impex = io::IoRegistry::instance().from_extension(extension) )
                if ( impex->can_open() )
                    return filename;
        }
    }

    return {};
}

void GlaxnimateWindow::Private::set_color_def(model::BrushStyle* def, bool secondary)
{
    model::Styler* target;
    QString what;
    if ( secondary )
    {
        target = ui.stroke_style_widget->shape();
        what = tr("Fill");
    }
    else
    {
        target = ui.fill_style_widget->shape();
        what = tr("Stroke");
    }

    if ( target )
    {
        auto old = target->use.get();

        if ( !def )
        {
            command::UndoMacroGuard macro(tr("Unlink %1 Color").arg(what), current_document.get());
            if ( auto col = qobject_cast<model::NamedColor*>(target->use.get()) )
                target->color.set_undoable(col->color.get());
            target->use.set_undoable(QVariant::fromValue(def));
            if ( old )
                old->remove_if_unused(false);
        }
        else
        {
            command::UndoMacroGuard macro(tr("Link %1 Color").arg(what), current_document.get());
            target->use.set_undoable(QVariant::fromValue(def));
            if ( old )
                old->remove_if_unused(false);
        }
    }

    set_brush_reference(def, secondary);
}

void GlaxnimateWindow::Private::set_brush_reference ( model::BrushStyle* sty, bool secondary )
{
    if ( secondary )
        widget_current_style->set_stroke_ref(sty);
    else
        widget_current_style->set_fill_ref(sty);

    if ( qobject_cast<model::Gradient*>(sty) )
        sty = nullptr;

    if ( secondary )
        secondary_brush = sty;
    else
        main_brush = sty;
}


void GlaxnimateWindow::Private::import_file()
{
    io::Options options = current_document->io_options();
    QString path = app::settings::get<QString>("open_save", "import_path");
    if ( !path.isEmpty() )
        options.path.setPath(path);

    ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
    if ( dialog.import_dialog() )
    {
        options = dialog.io_options();
        app::settings::set("open_save", "import_path", options.path.path());

        model::Document imported(options.filename);

        QFile file(options.filename);
        dialog_export_status->reset(options.format, options.filename);
        bool ok = options.format->open(file, options.filename, &imported, options.settings);
        if ( !ok )
        {
            show_warning(tr("Import File"), tr("Could not import %1").arg(options.filename));
            return;
        }

        /// \todo ask if comp
        parent->paste_document(&imported, tr("Import File"), true);
    }

}
