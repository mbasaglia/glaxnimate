#include "glaxnimate_window_p.hpp"

#include <QTemporaryFile>
#include <QDesktopServices>
#include <QFileDialog>
#include <QImageWriter>
#include <QtSvg/QSvgGenerator>

#include "widgets/dialogs/import_export_dialog.hpp"
#include "widgets/dialogs/io_status_dialog.hpp"
#include "io/lottie/lottie_html_format.hpp"
#include "app_info.hpp"
#include "model/layers/shape_layer.hpp"


void GlaxnimateWindow::Private::setup_document(const QString& filename)
{
    if ( !close_document() )
        return;

    current_document = std::make_unique<model::Document>(filename);

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

    property_model.set_document(current_document.get());
    property_model.set_object(current_document->main_composition());

    scene.set_document(current_document.get());

    ui.timeline_widget->set_document(current_document.get());

    // Scripting
    script_contexts.clear();

    // Title
    QObject::connect(current_document.get(), &model::Document::filename_changed, parent, &GlaxnimateWindow::refresh_title);
    QObject::connect(&current_document->undo_stack(), &QUndoStack::cleanChanged, parent, &GlaxnimateWindow::refresh_title);
    refresh_title();

    // Playback
    QObject::connect(current_document->main_composition(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &FrameControlsWidget::set_min);
    QObject::connect(current_document->main_composition(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &FrameControlsWidget::set_max);;
    QObject::connect(current_document->main_composition(), &model::MainComposition::fps_changed, ui.play_controls, &FrameControlsWidget::set_fps);
    QObject::connect(ui.play_controls, &FrameControlsWidget::frame_selected, current_document.get(), &model::Document::set_current_time);
    QObject::connect(current_document.get(), &model::Document::current_time_changed, ui.play_controls, &FrameControlsWidget::set_frame);
    QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, ui.play_controls, &FrameControlsWidget::set_record_enabled);
    QObject::connect(ui.play_controls, &FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);
}

void GlaxnimateWindow::Private::setup_document_new(const QString& filename)
{
    setup_document(filename);

    current_document->main_composition()->name.set(current_document->main_composition()->type_name_human());
    current_document->main_composition()->width.set(app::settings::get<int>("defaults", "width"));
    current_document->main_composition()->height.set(app::settings::get<int>("defaults", "height"));
    current_document->main_composition()->fps.set(app::settings::get<int>("defaults", "fps"));
    float duration = app::settings::get<float>("defaults", "duration");
    int out_point = current_document->main_composition()->fps.get() * duration;
    current_document->main_composition()->last_frame.set(out_point);


    auto layer = current_document->main_composition()->make_layer<model::ShapeLayer>();
    layer->last_frame.set(out_point);
    layer->name.set(layer->type_name_human());
    QPointF pos(
        current_document->main_composition()->width.get() / 2.0,
        current_document->main_composition()->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    model::Layer* ptr = layer.get();
    current_document->main_composition()->add_layer(std::move(layer), 0);

    QDir path = app::settings::get<QString>("open_save", "path");
    auto opts = current_document->io_options();
    opts.path = path;
    current_document->set_io_options(opts);

    ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
    ui.play_controls->set_range(0, out_point);
    view_fit();
}

bool GlaxnimateWindow::Private::setup_document_open(const io::Options& options)
{
    setup_document(options.filename);
    QFile file(options.filename);
    if ( !file.open(QFile::ReadOnly) )
        return false;

    dialog_import_status->reset(options.format, options.filename);
    bool ok = options.format->open(file, options.filename, current_document.get(), options.settings);

    app::settings::set<QString>("open_save", "path", options.path.absolutePath());

    if ( ok )
        most_recent_file(options.filename);

    view_fit();
    if ( !current_document->main_composition()->layers.empty() )
        ui.view_document_node->setCurrentIndex(document_node_model.node_index(&current_document->main_composition()->layers[0]));

    current_document->set_io_options(options);
    ui.play_controls->set_range(current_document->main_composition()->first_frame.get(), current_document->main_composition()->last_frame.get());
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
    if ( current_document && !current_document->undo_stack().isClean() )
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
            save_document(false, false);
        else if ( result == QMessageBox::Cancel )
            return false;

        // Prevent signals on the destructor
        current_document->undo_stack().clear();
    }

    document_node_model.clear_document();
    property_model.clear_document();
    scene.clear_document();
    ui.timeline_widget->clear_document();

    return true;
}


bool GlaxnimateWindow::Private::save_document(bool force_dialog, bool overwrite_doc)
{
    io::Options opts = current_document->io_options();

    if ( !opts.format || !opts.format->can_save() || !current_document->has_file() )
        force_dialog = true;

    if ( force_dialog )
    {
        ImportExportDialog dialog(current_document->io_options(), ui.centralwidget->parentWidget());

        if ( !dialog.export_dialog() )
            return false;

        opts = dialog.io_options();
    }

    QFile file(opts.filename);
    file.open(QFile::WriteOnly);
    dialog_export_status->reset(opts.format, opts.filename);
    if ( !opts.format->save(file, opts.filename, current_document.get(), opts.settings) )
        return false;

    most_recent_file(opts.filename);

    current_document->undo_stack().setClean();

    if ( overwrite_doc )
    {
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
        opts.format = io::ImportExport::factory().from_filename(filename);
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

void GlaxnimateWindow::Private::web_preview()
{
    QDir tempdir = QDir::temp();
    QString subdir = AppInfo::instance().slug();
    QString path = tempdir.filePath(subdir);

    if ( !tempdir.exists(subdir) )
        if ( !tempdir.mkpath(subdir) )
            path = "";

    QTemporaryFile tempf(path + "/XXXXXX.html");
    tempf.setAutoRemove(false);
    bool ok = tempf.open() && io::lottie::LottieHtmlFormat().save(
        tempf, tempf.fileName(), current_document.get(), {}
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


void GlaxnimateWindow::Private::save_frame_bmp()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, tr("Save Frame Image"));
    fd.setDirectory(current_document->io_options().path);
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

    QImage image(current_document->size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    current_document->main_composition()->paint(&painter, frame, model::DocumentNode::Recursive);
    if ( !image.save(fd.selectedFiles()[0]) )
        show_warning(tr("Render Frame"), tr("Could not save image"));
}


void GlaxnimateWindow::Private::save_frame_svg()
{
    int frame = current_document->current_time();
    QFileDialog fd(parent, tr("Save Frame Image"));
    fd.setDirectory(current_document->io_options().path);
    fd.setDefaultSuffix("svg");
    fd.selectFile(tr("Frame%1.svg").arg(frame));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setNameFilter(tr("Scalable Vector Graphics (*.svg)"));

    if ( fd.exec() == QDialog::Rejected )
        return;

    QFile file(fd.selectedFiles()[0]);
    if ( !file.open(QFile::WriteOnly) )
    {
        show_warning(tr("Render Frame"), tr("Could not save image"));
        return;
    }

    QSvgGenerator image;
    image.setSize(current_document->size());
    image.setOutputDevice(&file);
    image.setResolution(96);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    current_document->main_composition()->paint(&painter, frame, model::DocumentNode::Recursive);
}
