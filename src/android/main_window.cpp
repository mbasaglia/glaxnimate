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
#include "tools/base.hpp"
#include "widgets/dialogs/import_export_dialog.hpp"
#include "widgets/flow_layout.hpp"
#include "style/property_delegate.hpp"

#include "glaxnimate_app_android.hpp"
#include "android_file_picker.hpp"
#include "format_selection_dialog.hpp"
#include "document_opener.hpp"
#include "android_mime.hpp"
#include "sticker_pack_builder_dialog.hpp"
#include "scroll_area_event_filter.hpp"
#include "help_dialog.hpp"

using namespace glaxnimate::android;

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
    AndroidFilePicker file_picker;
    FormatSelectionDialog format_selector;
    DocumentOpener document_opener;

    FlowLayout* layout_tools = nullptr;
    FlowLayout* layout_actions = nullptr;
    QAction* action_undo = nullptr;
    QAction* action_redo = nullptr;
    QAction* action_toggle_widget_actions = nullptr;
    AndroidMime mime;
    StickerPackBuilderDialog telegram_export_dialog;
    style::PropertyDelegate property_delegate;

    Private(MainWindow* parent)
        : parent(parent),
          file_picker(parent),
          document_opener(parent)
    {
        ui.setupUi(parent);

        layout_actions = new FlowLayout(8, 32, 96);
        layout_actions->setSpacing(0);
        layout_actions->setMargin(0);
        ui.widget_actions->setLayout(layout_actions);

        layout_tools = new FlowLayout(8, 32, 96);
        layout_tools->setSpacing(0);
        layout_tools->setMargin(0);
        ui.layout_tools_container->addLayout(layout_tools);

        init_tools_ui();
        ui.canvas->set_tool_target(parent);
        ui.canvas->setScene(&scene);

        ui.button_expand_timeline->setVisible(false); // timeline is a bit weird atm
        ui.button_expand_timeline->setChecked(false);
        ui.button_expand_timeline->setIcon(GlaxnimateApp::theme_icon("expand-all"));

        ui.fill_style_widget->set_current_color(QColor("#3250b0"));
        ui.stroke_style_widget->set_color(QColor("#1d2848"));
        ui.stroke_style_widget->set_stroke_width(6);

        connect(
            QGuiApplication::primaryScreen(),
            &QScreen::orientationChanged,
            parent,
            [this]{adjust_size();}
        );

        setup_document_new();

        export_options.path = default_save_path();

        connect(&file_picker, &glaxnimate::android::AndroidFilePicker::open_selected, parent, [this](const QUrl& url, bool is_import){
            open_url(url, is_import);
        });
        connect(&file_picker, &glaxnimate::android::AndroidFilePicker::save_selected, parent,
            [this](const QUrl& url, bool is_export){
            save_url(url, is_export);
        });

        connect(&scene, &graphics::DocumentScene::node_user_selected, parent,
            [this](const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected){
                Q_UNUSED(deselected);
                if ( !selected.empty() )
                    this->parent->set_current_document_node(selected.back());
                else
                    this->parent->set_current_document_node(nullptr);
        });

        (new ScrollAreaEventFilter(ui.property_widget))->setParent(ui.property_widget);

        int side_width = ui.fill_style_widget->sizeHint().width();
        ui.stroke_style_widget->setMinimumWidth(side_width);
        ui.property_widget->setMinimumWidth(side_width);
    }

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
        clear_property_widgets();
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

    QToolButton* action_button(QAction* action)
    {
        auto btn = new QToolButton;
        btn->setDefaultAction(action);
        return btn;
    }

    void init_tools_ui()
    {
        // Tool Actions
        QActionGroup *tool_actions = new QActionGroup(parent);
        tool_actions->setExclusive(true);

        tools::Event event{ui.canvas, &scene, parent};
        tools::Tool* to_activate = nullptr;
        for ( const auto& grp : tools::Registry::instance() )
        {
            for ( const auto& tool : grp.second )
            {
                QAction* action = tool.second->get_action();
                layout_tools->addWidget(action_button(action));
                action->setParent(parent);
                action->setActionGroup(tool_actions);
                connect(action, &QAction::triggered, parent, &MainWindow::tool_triggered);

                if ( !to_activate )
                {
                    to_activate = tool.second.get();
                    action->setChecked(true);
                }
                tool.second->initialize(event);
            }
        }
        switch_tool(to_activate);
        action_toggle_widget_actions = view_action(
            GlaxnimateApp::theme_icon("overflow-menu"), tr("Views"),
            nullptr, ui.widget_actions, layout_tools, true
        );

        // Document actions
        document_action(GlaxnimateApp::theme_icon("document-new"), tr("New"), &Private::document_new);
        document_action(GlaxnimateApp::theme_icon("document-open"), tr("Open"), &Private::document_open);
        document_action(GlaxnimateApp::theme_icon("document-import"), tr("Import"), &Private::document_import);
        document_action(GlaxnimateApp::theme_icon("document-save"), tr("Save"), &Private::document_save);
        document_action(GlaxnimateApp::theme_icon("document-save-as"), tr("Save As"), &Private::document_save_as);
        document_action(GlaxnimateApp::theme_icon("document-export"), tr("Export"), &Private::document_export);
        document_action(GlaxnimateApp::theme_icon("document-send"), tr("Send to Telegram"), &Private::document_export_telegram);

        document_action_public(GlaxnimateApp::theme_icon("edit-cut"), tr("Cut"), &MainWindow::cut);
        document_action_public(GlaxnimateApp::theme_icon("edit-copy"), tr("Copy"), &MainWindow::copy);
        document_action_public(GlaxnimateApp::theme_icon("edit-paste"), tr("Paste"), &MainWindow::paste);
        document_action_public(GlaxnimateApp::theme_icon("edit-delete"), tr("Delete"), &MainWindow::delete_shapes);

        // Undo-redo
        action_undo = new QAction(GlaxnimateApp::theme_icon("edit-undo"), tr("Undo"), parent);
        layout_actions->addWidget(action_button(action_undo));
        action_redo = new QAction(GlaxnimateApp::theme_icon("edit-redo"), tr("Redo"), parent);
        layout_actions->addWidget(action_button(action_redo));

        // Views
        QActionGroup *view_actions = new QActionGroup(parent);
        view_actions->setExclusive(true);

        view_action(
            GlaxnimateApp::theme_icon("document-properties"), tr("Object Properties"),
            view_actions, ui.property_widget, layout_actions
        );

        view_action(
            GlaxnimateApp::theme_icon("player-time"), tr("Timeline"),
            view_actions, ui.time_container, layout_actions, true
        );

        view_action(
            GlaxnimateApp::theme_icon("fill-color"), tr("Fill Style"),
            view_actions, ui.fill_style_widget, layout_actions
        );

        view_action(
            GlaxnimateApp::theme_icon("object-stroke-style"), tr("Stroke Style"),
            view_actions, ui.stroke_style_widget, layout_actions
        );

        auto help = new QAction(GlaxnimateApp::theme_icon("question"), tr("Help"), parent);
        layout_actions->addWidget(action_button(help));
        connect(help, &QAction::triggered, parent, [this]{
            HelpDialog(parent).exec();
        });

    }

    void document_action(const QIcon& icon, const QString& text, void (Private::* func)())
    {
        QAction* action = new QAction(icon, text, parent);
        layout_actions->addWidget(action_button(action));
        connect(action, &QAction::triggered, parent, [this, func]{
            (this->*func)();
        });
    }

    template<class Callback>
    void document_action_public(const QIcon& icon, const QString& text, Callback func)
    {
        QAction* action = new QAction(icon, text, parent);
        layout_actions->addWidget(action_button(action));
        connect(action, &QAction::triggered, parent, [this, func]{
            (parent->*func)();
        });
    }

    QAction* view_action(const QIcon& icon, const QString& text, QActionGroup* group,
                         QWidget* target, FlowLayout* container, bool checked = false)
    {
        QAction* action = new QAction(icon, text, parent);
        action->setCheckable(true);
        action->setChecked(checked);
        target->setVisible(checked);
        action->setActionGroup(group);
        connect(action, &QAction::toggled, target, &QWidget::setVisible);
        container->addWidget(action_button(action));
        return action;
    }

    bool save_document(bool force_dialog, bool export_opts)
    {
        io::Options opts = export_opts ? export_options : current_document->io_options();

        if ( export_opts || !opts.format || !opts.format->can_save() || !current_document_has_file || opts.filename.isEmpty() )
            force_dialog = true;

        if ( force_dialog )
        {
            if ( export_opts || !opts.format )
            {
                format_selector.setFocus();
                if ( !format_selector.exec() )
                    return false;
                opts.format = format_selector.format();
            }

            QString suggestion;
            if ( !opts.filename.isEmpty() )
                suggestion = opts.filename;
            else
                suggestion = tr("Animation.%1").arg(opts.format ? opts.format->extensions()[0] : "rawr");

            if ( file_picker.select_save(suggestion, export_opts) )
            {
                if ( export_opts )
                    export_options = opts;
                else
                    current_document->set_io_options(opts);

                return true;
            }

            ImportExportDialog dialog(opts, parent);

            if ( !dialog.export_dialog() )
                return false;

            opts = dialog.io_options();
        }

        return save_url(opts.filename, export_opts);
    }

    void save_document_set_opts(const io::Options& opts, bool export_opts)
    {
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

            if ( !file_picker.select_open(false) )
            {
                // Ugly widget as fallback
                ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
                if ( dialog.import_dialog() )
                {
                    options = dialog.io_options();
                    clear_document();
                    QFile file(options.filename);
                    current_document = std::make_unique<model::Document>(options.filename);
                    options.format->open(file, options.filename, current_document.get(), options.settings);
                    current_document->set_io_options(options);
                    setup_document_open();

                }
                else
                {
                    setup_document_new();
                }
            }
        }
    }

    void document_import()
    {
        if ( !file_picker.select_open(true) )
        {
            // Ugly widget as fallback
            ImportExportDialog dialog(current_document->io_options(), ui.centralwidget->parentWidget());
            if ( dialog.import_dialog() )
            {
                io::Options options = dialog.io_options();


                model::Document imported(options.filename);
                QFile file(options.filename);
                bool ok = options.format->open(file, options.filename, &imported, options.settings);

                if ( !ok )
                {
                    QMessageBox::warning(parent, tr("Import File"), tr("Could not import file"));
                    return;
                }

                parent->paste_document(&imported, tr("Import File"), true);

            }
        }
    }

    void setup_document_open()
    {
        current_document_has_file = true;

        export_options = current_document->io_options();
        export_options.filename = "";
        setup_document();
    }

    void document_save()
    {
        save_document(false, false);
    }

    void document_save_as()
    {
        save_document(true, false);
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
        telegram_export_dialog.set_current_file(current_document.get());
        telegram_export_dialog.exec();
    }

    void adjust_size()
    {
        int mins;
        qreal tool_layout_extent;

        Qt::Orientation toolbar_orientation;

        QSize screen_size = parent->size();

        if ( screen_size.width() > screen_size.height() )
        {
            toolbar_orientation = Qt::Vertical;
            mins = screen_size.height();
        }
        else
        {
            toolbar_orientation = Qt::Horizontal;
            mins = screen_size.width();
        }

        int button_w = qRound(mins * 0.08);
        QSize button_size(button_w, button_w);

        for ( QToolButton* btn : parent->findChildren<QToolButton*>() )
            btn->setIconSize(button_size);

        ui.slider_frame->setFixedHeight(button_w);

        ui.widget_tools_container_side->setVisible(false);
        ui.widget_tools_container_bottom->setVisible(false);

        tool_layout_extent = mins;
        if ( toolbar_orientation == Qt::Vertical )
            tool_layout_extent *= 0.7;
        tool_layout_extent = qRound(tool_layout_extent/9.);

        layout_actions->set_orientation(toolbar_orientation);
        layout_tools->set_orientation(toolbar_orientation);
        QSize tool_layout_size(tool_layout_extent, tool_layout_extent);
        layout_tools->set_fixed_item_size(tool_layout_size);
        layout_actions->set_fixed_item_size(tool_layout_size);

        if ( toolbar_orientation == Qt::Horizontal )
        {
            ui.widget_tools_container_bottom->setLayout(ui.layout_tools_container);
            ui.layout_tools_container->setDirection(QBoxLayout::TopToBottom);
            ui.widget_tools_container_bottom->setVisible(true);
        }
        else
        {
            ui.layout_tools_container->setDirection(QBoxLayout::RightToLeft);
            ui.widget_tools_container_side->setLayout(ui.layout_tools_container);
            ui.widget_tools_container_side->setVisible(true);
        }
    }

    void open_url(const QUrl& url, bool is_import)
    {
        qDebug() << "open_url" << url << is_import;
        if ( !url.isValid() )
            return;

        if ( is_import )
        {
            auto imported = document_opener.open(url);
            if ( imported )
                parent->paste_document(imported.get(), tr("Import File"), true);
            return;
        }

        clear_document();
        current_document = document_opener.open(url);
        if ( current_document )
            setup_document_open();
        else
            setup_document_new();
    }

    bool save_url(const QUrl& url, bool export_opts)
    {
        if ( !url.isValid() )
            return false;

        io::Options options = export_opts ? export_options : current_document->io_options();
        if ( document_opener.save(url, current_document.get(), options) )
        {
            save_document_set_opts(options, export_opts);
            return true;
        }

        return false;
    }

    void clear_property_widgets()
    {
        while ( QLayoutItem *child = ui.property_widget_layout->takeAt(0) )
        {
            delete child->widget();
            delete child;
        }
    }

    void add_property_widgets_object(model::Object* node)
    {
        using Traits = model::PropertyTraits;
        std::vector<std::pair<QWidget*, model::BaseProperty*>> props;

        for ( const auto& prop : node->properties() )
        {
            auto traits = prop->traits();
            if ( traits.type == Traits::ObjectReference || (traits.flags & Traits::List) )
            {
                continue;
            }
            else if ( traits.type == Traits::Object )
            {
                add_property_widgets_object(prop->value().value<model::Object*>());
            }
            else if ( !(traits.flags & Traits::Visual)  )
            {
                continue;
            }
            else if ( auto wid = property_delegate.editor_from_property(prop, nullptr) )
            {
                props.emplace_back(wid, prop);
                auto lab = new QLabel(prop->name());
                lab->setBuddy(wid);
                ui.property_widget_layout->addWidget(lab);
                property_delegate.set_editor_data(wid, prop);
                ui.property_widget_layout->addWidget(wid);
            }
        }

        connect(node, &model::Object::visual_property_changed, props[0].first, [props, this]{
            for ( const auto& p : props )
                property_delegate.set_editor_data(p.first, p.second);
        });

    }

    void add_property_widgets(model::VisualNode* node)
    {
        QLabel* label = new QLabel(node->object_name());
        ui.property_widget_layout->addWidget(label);
        connect(node, &model::DocumentNode::name_changed, label, &QLabel::setText);

        if ( auto grp = node->cast<model::Group>() )
        {
            for ( const auto& child : grp->shapes )
            {
                if ( !child->is_instance<model::Group>() && !child->is_instance<model::Styler>() )
                    add_property_widgets(child.get());
            }
        }

        add_property_widgets_object(node);
    }


    void set_property_widgets(model::VisualNode* node)
    {
        clear_property_widgets();
        if ( node )
        {
            add_property_widgets(node);
            ui.property_widget->setMinimumWidth(ui.property_widgets_inner->sizeHint().width() + 6);
        }
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d(std::make_unique<Private>(this))
{
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
    if ( node == d->current_node )
        return;

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

    d->ui.stroke_style_widget->set_shape(d->current_stroke);
    d->ui.fill_style_widget->set_shape(d->current_fill);


    d->set_property_widgets(node);
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

std::vector<io::mime::MimeSerializer *> MainWindow::supported_mimes() const
{
    return {
        &d->mime
    };
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

void MainWindow::set_selection(const std::vector<model::VisualNode*>& selected)
{
    d->scene.user_select(selected, graphics::DocumentScene::Replace);
}
