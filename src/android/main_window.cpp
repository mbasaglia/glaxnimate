#include "main_window.hpp"
#include "ui_main_window.h"

#include <QMessageBox>
#include <QPointer>
#include <QScreen>
#include <QMenu>
#include <QStandardPaths>

#include "model/document.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/lottie/tgs_format.hpp"

#include "graphics/document_scene.hpp"

#include "widgets/dialogs/import_export_dialog.hpp"
#include "tools/base.hpp"
#include "glaxnimate_app_android.hpp"
#include "telegram_intent.hpp"
#include "android_file_picker.hpp"

class MainWindow::Private
{
public:
    MainWindow* parent;
    Ui::MainWindow ui;
    graphics::DocumentScene scene;
    std::unique_ptr<model::Document> current_document;
    tools::Tool* active_tool = nullptr;

    QPointer<model::BrushStyle> main_brush;
    QPointer<model::BrushStyle> secondary_brush;
    model::Composition* comp = nullptr;
    QPointer<model::VisualNode> current_node;
    QPointer<model::Fill> current_fill;
    QPointer<model::Stroke> current_stroke;

    io::Options export_options;
    bool current_document_has_file = false;
    glaxnimate::android::AndroidFilePicker file_picker;

    QAction* action_undo = nullptr;
    QAction* action_redo = nullptr;
    QAction* action_toolbar = nullptr;

    void setup_document_new()
    {
        clear_document();

        current_document = std::make_unique<model::Document>("");
        current_document->main()->width.set(512);
        current_document->main()->height.set(512);
        current_document->main()->animation->last_frame.set(180);
        current_document->main()->fps.set(60);

        auto opts = current_document->io_options();
        opts.format = io::glaxnimate::GlaxnimateFormat::instance();
        opts.path = default_save_path();
        current_document->set_io_options(opts);

        auto layer = std::make_unique<model::Layer>(current_document.get());
        layer->animation->last_frame.set(180);
        layer->name.set(layer->type_name_human());
        QPointF pos(
            current_document->main()->width.get() / 2.0,
            current_document->main()->height.get() / 2.0
        );
        layer->transform.get()->anchor_point.set(pos);
        layer->transform.get()->position.set(pos);

        current_document->main()->shapes.insert(std::move(layer), 0);

        setup_document();
    }

    void clear_document()
    {
        comp = nullptr;
        scene.set_document(nullptr);
        ui.timeline_widget->set_document(nullptr);
        action_redo->setEnabled(false);
        action_undo->setEnabled(false);
        current_document.reset();
        current_document_has_file = false;
    }

    void setup_document()
    {
        scene.set_document(current_document.get());
        comp = current_document->main();
        current_document->set_record_to_keyframe(true);

        // Undo Redo
        connect(action_undo, &QAction::triggered, &current_document->undo_stack(), &QUndoStack::undo);
        connect(&current_document->undo_stack(), &QUndoStack::canUndoChanged, action_undo, &QAction::setEnabled);
        connect(action_redo, &QAction::triggered, &current_document->undo_stack(), &QUndoStack::redo);
        connect(&current_document->undo_stack(), &QUndoStack::canRedoChanged, action_redo, &QAction::setEnabled);

        // play controls
        auto first_frame = current_document->main()->animation->first_frame.get();
        auto last_frame = current_document->main()->animation->last_frame.get();
        ui.play_controls->set_range(first_frame, last_frame);
        ui.play_controls->set_record_enabled(current_document->record_to_keyframe());
        QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &FrameControlsWidget::set_min);
        QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &FrameControlsWidget::set_max);;
        QObject::connect(current_document->main(), &model::MainComposition::fps_changed, ui.play_controls, &FrameControlsWidget::set_fps);
        QObject::connect(ui.play_controls, &FrameControlsWidget::frame_selected, current_document.get(), &model::Document::set_current_time);
        QObject::connect(current_document.get(), &model::Document::current_time_changed, ui.play_controls, &FrameControlsWidget::set_frame);
        QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, ui.play_controls, &FrameControlsWidget::set_record_enabled);
        QObject::connect(ui.play_controls, &FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);

        // slider
        ui.slider_frame->setMinimum(first_frame);
        ui.slider_frame->setMaximum(last_frame);
        ui.slider_frame->setValue(first_frame);
        QObject::connect(ui.slider_frame, &QSlider::valueChanged, current_document.get(), &model::Document::set_current_time);
        QObject::connect(current_document.get(), &model::Document::current_time_changed, ui.slider_frame, &QSlider::setValue);

        // timeline
        ui.timeline_widget->reset_view();
        ui.timeline_widget->set_document(current_document.get());
        ui.timeline_widget->set_composition(comp);
    }

    void switch_tool(tools::Tool* tool)
    {
        if ( !tool || tool == active_tool )
            return;

        if ( !tool->get_action()->isChecked() )
            tool->get_action()->setChecked(true);
/*
        if ( active_tool )
        {
            for ( const auto& widget : tool_widgets[active_tool->id()] )
            {
                widget->setVisible(false);
                widget->setEnabled(false);
            }

            for ( const auto& action : tool_actions[active_tool->id()] )
            {
                action->setEnabled(false);
            }
        }


        for ( const auto& widget : tool_widgets[tool->id()] )
        {
            widget->setVisible(true);
            widget->setEnabled(true);
        }

        for ( const auto& action : tool_actions[tool->id()] )
        {
            action->setEnabled(true);
        }
*/
        active_tool = tool;
        scene.set_active_tool(tool);
        ui.canvas->set_active_tool(tool);
        /*
        ui.tool_settings_widget->setCurrentWidget(tool->get_settings_widget());
        if ( active_tool->group() == tools::Registry::Draw || active_tool->group() == tools::Registry::Shape )
            widget_current_style->clear_gradients();*/
    }

    void init_tools_ui()
    {
        // Tool Actions
        QActionGroup *tool_actions = new QActionGroup(parent);
        tool_actions->setExclusive(true);

        for ( const auto& grp : tools::Registry::instance() )
        {
            for ( const auto& tool : grp.second )
            {
                QAction* action = tool.second->get_action();
                action->setParent(parent);
                action->setActionGroup(tool_actions);
                connect(action, &QAction::triggered, parent, &MainWindow::tool_triggered);

                ui.toolbar_tools->addAction(action);

                if ( !active_tool )
                {
                    active_tool = tool.second.get();
                    action->setChecked(true);
                }
            }
        }

        // Undo-redo

        action_undo = new QAction(GlaxnimateApp::theme_icon("edit-undo"), tr("Undo"), parent);
        ui.toolbar_tools->addAction(action_undo);
        action_redo = new QAction(GlaxnimateApp::theme_icon("edit-redo"), tr("Redo"), parent);
        ui.toolbar_tools->addAction(action_redo);

        // Document actions
        document_action(GlaxnimateApp::theme_icon("document-new"), tr("New"), &Private::document_new);
        document_action(GlaxnimateApp::theme_icon("document-open"), tr("Open"), &Private::document_open);
        document_action(GlaxnimateApp::theme_icon("document-save"), tr("Save"), &Private::document_save);
        document_action(GlaxnimateApp::theme_icon("document-export"), tr("Export"), &Private::document_export);
        document_action(GlaxnimateApp::theme_icon("document-send"), tr("Send to Telegram"), &Private::document_export_telegram);

        // Views
        ui.toolbar_actions->addSeparator();

        QActionGroup *view_actions = new QActionGroup(parent);
        view_actions->setExclusive(true);

        action_toolbar = view_action(
            GlaxnimateApp::theme_icon("overflow-menu"), tr("Views"),
            nullptr, ui.toolbar_actions, ui.toolbar_tools
        );

        view_action(
            GlaxnimateApp::theme_icon("player-time"), tr("Timeline"),
            view_actions, ui.time_container, ui.toolbar_actions, true
        );

        view_action(
            GlaxnimateApp::theme_icon("fill-color"), tr("Fill Style"),
            view_actions, ui.fill_style_widget, ui.toolbar_actions
        );

        view_action(
            GlaxnimateApp::theme_icon("object-stroke-style"), tr("Stroke Style"),
            view_actions, ui.stroke_style_widget, ui.toolbar_actions
        );
    }

    QAction* view_action(const QIcon& icon, const QString& text, QActionGroup* group,
                         QWidget* target, QWidget* add_to, bool checked = false)
    {
        QAction* action = new QAction(icon, text, parent);
        action->setCheckable(true);
        action->setChecked(checked);
        target->setVisible(checked);
        action->setActionGroup(group);
        connect(action, &QAction::toggled, target, &QWidget::setVisible);
        add_to->addAction(action);
        return action;
    }

    bool save_document(bool force_dialog, bool export_opts)
    {
        io::Options opts = export_opts ? export_options : current_document->io_options();

        if ( !opts.format || !opts.format->can_save() || !current_document_has_file || opts.filename.isEmpty() )
            force_dialog = true;

        if ( force_dialog )
        {
            QString suggestion;
            if ( !opts.filename.isEmpty() )
                suggestion = opts.filename;
            else
                suggestion = tr("Animation.%1").arg(opts.format ? opts.format->extensions()[0] : "rawr");

            if ( !file_picker.select_save(suggestion, export_opts) )
            {
                ImportExportDialog dialog(opts, parent);

                if ( !dialog.export_dialog() )
                    return false;

                opts = dialog.io_options();
            }
        }

        if ( opts.filename.startsWith("content:/") )
        {
            return save_url(opts.filename, export_opts);
        }
        else
        {
            QFile file(opts.filename);
            return do_save_document(opts, export_opts, file);
        }
    }

    bool do_save_document(const io::Options& opts, bool export_opts, QIODevice& file)
    {
        if ( !opts.format->save(file, opts.filename, current_document.get(), opts.settings) )
            return false;

        if ( export_opts )
        {
            export_options = opts;
        }
        else
        {
            current_document->set_io_options(opts);
            current_document->undo_stack().setClean();
            current_document_has_file = true;
        }

        return true;
    }

    bool close_document()
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

        if ( active_tool )
            active_tool->close_document_event({ui.canvas, &scene, parent});

        clear_document();

        return true;
    }

    void document_new()
    {
        if ( close_document() )
            setup_document_new();
    }

    void document_open()
    {
        io::Options options = current_document->io_options();

        if ( close_document() )
        {
            setup_document_new();

            if ( !file_picker.select_open() )
            {
                // Ugly widget as fallback
                ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
                if ( dialog.import_dialog() )
                {
                    QFile file(dialog.io_options().filename);
                    setup_document_open(dialog.io_options(), file);
                }
                else
                {
                    setup_document_new();
                }
            }
        }
    }

    bool setup_document_open(const io::Options& options, QIODevice& file)
    {
        clear_document();

        current_document_has_file = true;

        current_document = std::make_unique<model::Document>(options.filename);

        bool ok = options.format->open(file, options.filename, current_document.get(), options.settings);

        current_document->set_io_options(options);
        export_options = options;
        export_options.filename = "";

        setup_document();

        return ok;
    }

    void document_save()
    {
        save_document(false, false);
    }

    void document_export()
    {
        save_document(true, true);
    }


    QDir default_save_path()
    {
        if ( file_picker.get_permissions() )
            return QDir("/storage/emulated/0/Movies");

        return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    }

    void document_export_telegram()
    {
        if ( !current_document->undo_stack().isClean() )
            save_document(false, false);

        QStringList filenames;
        QStringList emoji;

        QString filename = default_save_path().filePath("test123.tgs");
        QFile file(filename);
        if ( !io::lottie::TgsFormat().save(file, filename, current_document.get(), {}) )
            QMessageBox::warning(parent, tr("Export Stickers"), tr("Cannot save as TGS"));
        filenames << filename;
        emoji << "😀";

        auto result = TelegramIntent().send_stickers(filenames, emoji);
        if ( !result )
            QMessageBox::warning(parent, tr("Export Stickers"), result.message());

    }

    void document_action(const QIcon& icon, const QString& text, void (Private::* func)())
    {
        QAction* action = new QAction(icon, text, parent);
        ui.toolbar_actions->addAction(action);
        connect(action, &QAction::triggered, parent, [this, func]{
            (this->*func)();
        });
    }

    void adjust_size()
    {
        int mins;

        Qt::ToolBarArea toolbar_area;
        Qt::Orientation toolbar_orientation;

        if ( parent->width() > parent->height() )
        {
            toolbar_area = Qt::LeftToolBarArea;
            toolbar_orientation = Qt::Vertical;
            mins = parent->height();
        }
        else
        {
            toolbar_area = Qt::BottomToolBarArea;
            toolbar_orientation = Qt::Horizontal;
            mins = parent->width();
        }

        parent->addToolBar(toolbar_area, ui.toolbar_tools);
        parent->addToolBarBreak(toolbar_area);
        parent->addToolBar(toolbar_area, ui.toolbar_actions);
        ui.toolbar_actions->setVisible(action_toolbar->isChecked());

        int button_w = qRound(mins * 0.08);
        QSize button_size(button_w, button_w);

        for ( QToolButton* btn : parent->findChildren<QToolButton*>() )
            btn->setIconSize(button_size);

        for ( QToolBar* bar : parent->findChildren<QToolBar*>() )
        {
            bar->setIconSize(button_size);
            bar->setOrientation(toolbar_orientation);
        }

        ui.slider_frame->setFixedHeight(button_w);
    }

    void open_url(const QUrl& url)
    {
        if ( !url.isValid() )
            return;

        QString path = url.path();
        QFileInfo finfo(path);
        QString extension = finfo.suffix();
        io::Options options;
        options.format = io::IoRegistry::instance().from_extension(extension);
        if ( !options.format )
        {
            QMessageBox::warning(parent, tr("Open File"), tr("Unknown file type"));
            return;
        }

        if ( url.isLocalFile() )
        {
            options.filename = path;
            options.path = finfo.absoluteDir();
            QFile file(options.filename);
            setup_document_open(options, file);
        }
        else
        {
            options.filename = url.toString();
            QByteArray data = file_picker.read_content_uri(url);
            QBuffer buf(&data);
            setup_document_open(options, buf);
        }
    }

    bool save_url(const QUrl& url, bool export_opts)
    {
        if ( !url.isValid() )
            return false;

        QString path = url.path();
        QFileInfo finfo(path);
        QString extension = finfo.suffix();
        io::Options options;
        options.format = io::IoRegistry::instance().from_extension(extension);
        if ( !options.format )
        {
            QMessageBox::warning(parent, tr("Save File"), tr("Unknown file type"));
            return false;
        }

        if ( url.isLocalFile() )
        {
            options.filename = path;
            options.path = finfo.absoluteDir();

            QFile file(options.filename);
            return do_save_document(options, export_opts, file);
        }
        else
        {
            options.filename = url.toString();
            QByteArray data;
            QBuffer buf(&data);
            bool ok = do_save_document(options, export_opts, buf);
            if ( !ok || !file_picker.write_content_uri(url, data) )
            {
                QMessageBox::warning(parent, tr("Save File"), tr("Could not save the file"));
                return false;
            }
            return true;
        }

    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d(std::make_unique<Private>())
{
    d->parent = this;
    d->ui.setupUi(this);

    d->init_tools_ui();
    d->ui.canvas->set_tool_target(this);
    d->ui.canvas->setScene(&d->scene);

    d->ui.button_expand_timeline->setVisible(false); // timeline is a bit weird atm
    d->ui.button_expand_timeline->setChecked(false);
    d->ui.button_expand_timeline->setIcon(GlaxnimateApp::theme_icon("expand-all"));

    d->ui.fill_style_widget->set_current_color(QColor("#3250b0"));
    d->ui.stroke_style_widget->set_color(QColor("#1d2848"));
    d->ui.stroke_style_widget->set_stroke_width(6);

    connect(
        QGuiApplication::primaryScreen(),
        &QScreen::orientationChanged,
        this,
        [this]{d->adjust_size();}
    );

    d->setup_document_new();

    d->export_options.path = d->default_save_path();

    connect(&d->file_picker, &glaxnimate::android::AndroidFilePicker::open_selected, this, [this](const QUrl& url){
        d->open_url(url);
    });
    connect(&d->file_picker, &glaxnimate::android::AndroidFilePicker::save_selected, this,
        [this](const QUrl& url, bool is_export){
        d->save_url(url, is_export);
    });
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

model::Document* MainWindow::document() const
{
    return d->current_document.get();
}


model::Composition* MainWindow::current_composition() const
{
    return d->comp;
}

model::VisualNode* MainWindow::current_document_node() const
{
    return d->current_node;
}

QColor MainWindow::current_color() const
{
    return d->ui.fill_style_widget->current_color();
}

void MainWindow::set_current_color(const QColor& c)
{
    d->ui.fill_style_widget->set_current_color(c);
}

QColor MainWindow::secondary_color() const
{
    return d->ui.stroke_style_widget->current_color();
}

void MainWindow::set_secondary_color(const QColor& c)
{
    d->ui.stroke_style_widget->set_color(c);
}

QPen MainWindow::current_pen_style() const
{
    return d->ui.stroke_style_widget->pen_style();
}

qreal MainWindow::current_zoom() const
{
    return d->ui.canvas->get_zoom_factor();
}

model::BrushStyle* MainWindow::linked_brush_style(bool secondary) const
{
    if ( secondary )
        return d->secondary_brush;
    return d->main_brush;
}

void MainWindow::set_current_document_node(model::VisualNode* node)
{
    d->current_node = node;
    d->current_fill = nullptr;
    d->current_stroke = nullptr;
    d->main_brush = nullptr;
    d->secondary_brush = nullptr;

    if ( node )
    {
        d->scene.user_select({node}, graphics::DocumentScene::Replace);
        auto grp = node->cast<model::Group>();
        if ( !grp )
            grp = node->docnode_parent()->cast<model::Group>();
        if ( grp )
        {
            for ( const auto& sh : grp->shapes )
            {
                if ( auto fill = sh->cast<model::Fill>() )
                    d->current_fill = fill;
                else if ( auto stroke = sh->cast<model::Stroke>() )
                    d->current_stroke = stroke;
            }
        }

        if ( d->current_fill )
            d->main_brush = d->current_fill->use.get();
        if ( d->current_stroke )
            d->secondary_brush = d->current_stroke->use.get();
    }
}

void MainWindow::set_current_composition(model::Composition* comp)
{
    d->comp = comp ? comp : d->current_document->main();
    d->scene.set_composition(comp);
}

void MainWindow::switch_tool(tools::Tool* tool)
{
    d->switch_tool(tool);
}

std::vector<model::VisualNode*> MainWindow::cleaned_selection() const
{
    return d->scene.cleaned_selection();
}


void MainWindow::tool_triggered(bool checked)
{
    if ( checked )
        d->switch_tool(static_cast<QAction*>(sender())->data().value<tools::Tool*>());
}

void MainWindow::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);
    d->adjust_size();
}
