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

#include "command/undo_macro_guard.hpp"
#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"

#include "graphics/document_scene.hpp"
#include "tools/base.hpp"
#include "item_models/document_node_model.hpp"
#include "widgets/dialogs/import_export_dialog.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/layer_view.hpp"
#include "style/property_delegate.hpp"
#include "utils/pseudo_mutex.hpp"

#include "android_file_picker.hpp"
#include "format_selection_dialog.hpp"
#include "document_opener.hpp"
#include "sticker_pack_builder_dialog.hpp"
#include "scroll_area_event_filter.hpp"
#include "help_dialog.hpp"
#include "timeline_slider.hpp"
#include "better_toolbox_widget.hpp"


using namespace glaxnimate::android;

class MainWindow::Private
{
public:
    MainWindow* parent;
    Ui::MainWindow ui;
    gui::graphics::DocumentScene scene;
    std::unique_ptr<model::Document> current_document;
    gui::tools::Tool* active_tool = nullptr;

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

    QHBoxLayout* layout_tools = nullptr;
    QHBoxLayout* layout_actions = nullptr;
    QHBoxLayout* layout_edit_actions = nullptr;
    QAction* action_undo = nullptr;
    QAction* action_redo = nullptr;
    StickerPackBuilderDialog telegram_export_dialog;
    gui::style::PropertyDelegate property_delegate;
    QActionGroup *view_actions = nullptr;
    TimelineSlider* timeline_slider;
    std::vector<QSpacerItem*> toolbar_spacers;
    gui::LayerView* layer_view = nullptr;
    gui::item_models::DocumentNodeModel document_node_model;
    utils::PseudoMutex updating_selection;

    Private(MainWindow* parent)
        : parent(parent),
          file_picker(parent),
          document_opener(parent)
    {
        ui.setupUi(parent);

        timeline_slider = new TimelineSlider(ui.time_container);
        ui.time_container_layout->insertWidget(1, timeline_slider);

        layout_actions = init_toolbar_layout();
        ui.widget_actions->setLayout(layout_actions);

        layout_tools = init_toolbar_layout();
        ui.layout_tools_container->addLayout(layout_tools);

        init_toolbar_tools();
        init_toolbar_actions();
        init_toolbar_edit();

        ui.canvas->set_tool_target(parent);
        ui.canvas->setScene(&scene);

        ui.fill_style_widget->set_current_color(QColor("#3250b0"));
        ui.stroke_style_widget->set_color(QColor("#1d2848"));
        ui.stroke_style_widget->set_stroke_width(6);

        connect(
            QGuiApplication::primaryScreen(),
            &QScreen::primaryOrientationChanged,
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

        connect(&scene, &gui::graphics::DocumentScene::node_user_selected, parent, [this](const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected){
            this->parent->update_selection(selected, deselected);
            if ( !selected.empty() )
                this->parent->set_current_document_node(selected.back());
            else
                this->parent->set_current_document_node(nullptr);
        });
        connect(layer_view, &gui::LayerView::selection_changed, parent, [this](const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected){
            this->parent->update_selection(selected, deselected);
        });
        connect(layer_view, &gui::LayerView::current_node_changed, parent, [this](model::VisualNode* selected){
            this->parent->set_current_document_node(selected);
        });

        (new ScrollAreaEventFilter(ui.property_widget))->setParent(ui.property_widget);

        int side_width = ui.fill_style_widget->sizeHint().width();
        ui.stroke_style_widget->setMinimumWidth(side_width);
        ui.property_widget->setMinimumWidth(400);

        adjust_size();
    }

    QHBoxLayout* init_toolbar_layout()
    {
        auto lay = new QHBoxLayout();
        lay->setSpacing(0);
        lay->setMargin(0);
        return lay;
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
        if ( active_tool )
            active_tool->close_document_event({ui.canvas, &scene, parent});
        ui.play_controls->pause();
        document_node_model.set_document(nullptr);
        scene.set_document(nullptr);
        action_redo->setEnabled(false);
        action_undo->setEnabled(false);        
        clear_property_widgets();
        ui.stroke_style_widget->set_shape(nullptr);
        ui.fill_style_widget->set_shape(nullptr);
        comp = nullptr;
        current_document.reset();
        current_document_has_file = false;
    }

    void setup_document()
    {
        scene.set_document(current_document.get());
        document_node_model.set_document(current_document.get());
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
        QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::first_frame_changed, ui.play_controls, &gui::FrameControlsWidget::set_min);
        QObject::connect(current_document->main()->animation.get(), &model::AnimationContainer::last_frame_changed, ui.play_controls, &gui::FrameControlsWidget::set_max);;
        QObject::connect(current_document->main(), &model::MainComposition::fps_changed, ui.play_controls, &gui::FrameControlsWidget::set_fps);
        QObject::connect(ui.play_controls, &gui::FrameControlsWidget::frame_selected, current_document.get(), &model::Document::set_current_time);
        QObject::connect(current_document.get(), &model::Document::current_time_changed, ui.play_controls, &gui::FrameControlsWidget::set_frame);
        QObject::connect(current_document.get(), &model::Document::record_to_keyframe_changed, ui.play_controls, &gui::FrameControlsWidget::set_record_enabled);
        QObject::connect(ui.play_controls, &gui::FrameControlsWidget::record_toggled, current_document.get(), &model::Document::set_record_to_keyframe);

        // slider
        timeline_slider->setMinimum(first_frame);
        timeline_slider->setMaximum(last_frame);
        timeline_slider->setValue(first_frame);
        QObject::connect(timeline_slider, &QAbstractSlider::valueChanged, current_document.get(), &model::Document::set_current_time);
        QObject::connect(current_document.get(), &model::Document::current_time_changed, timeline_slider, &QAbstractSlider::setValue);

        // Views
        layer_view->set_composition(comp);

    }

    void switch_tool(gui::tools::Tool* tool)
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


    QToolButton* action_button_exclusive_opt(QAction* action)
    {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        auto btn = new QToolButton;
        btn->setIcon(action->icon());
        btn->setText(action->text());
        btn->setCheckable(action->isCheckable());
        connect(action, &QAction::toggled, btn, &QAbstractButton::setChecked);

        connect(btn, &QAbstractButton::clicked, action, [action](bool b){
            if ( b )
                action->trigger();
            else
                action->setChecked(false);
        });
        return btn;
#else
        return action_button(action);
#endif
    }

    QMenu* action_menu(const QIcon& icon, const QString& label, QHBoxLayout* container)
    {
        QMenu *menu = new QMenu(parent);
        menu->setTitle(label);
        menu->setIcon(icon);

        QToolButton* button = new QToolButton;
        button->setMenu(menu);
        button->setIcon(icon);
        button->setText(label);
        button->setPopupMode(QToolButton::InstantPopup);
        container->addWidget(button);

        return menu;
    }

    std::vector<QAction*> tool_actions(const gui::tools::Registry::mapped_type& group, QActionGroup *tool_actions, gui::tools::Tool*& to_activate, const gui::tools::Event& event)
    {
        std::vector<QAction*> ret;

        for ( const auto& tool : group )
        {
            QAction* action = tool.second->get_action();
            ret.push_back(action);
            action->setParent(parent);
            action->setActionGroup(tool_actions);
            connect(action, &QAction::triggered, parent, &MainWindow::tool_triggered);

            if ( !to_activate )
            {
                to_activate = tool.second.get();
                action->setChecked(true);
            }
            tool.second->retranslate();
            tool.second->initialize(event);
        }

        return ret;
    }

    void init_toolbar_tools()
    {
        // Tool Actions
        QActionGroup *tool_actions_grp = new QActionGroup(parent);
        tool_actions_grp->setExclusive(true);

        gui::tools::Event event{ui.canvas, &scene, parent};
        gui::tools::Tool* to_activate = nullptr;

        for ( auto action: tool_actions(gui::tools::Registry::instance()[gui::tools::Registry::Core], tool_actions_grp, to_activate, event) )
            layout_tools->addWidget(action_button(action));

        std::map<int, const char*> icons = {
            {gui::tools::Registry::Draw, "draw-brush"},
            {gui::tools::Registry::Shape, "shapes"},
        };

        for ( const auto& grp : gui::tools::Registry::instance() )
        {
            if ( grp.first == gui::tools::Registry::Core )
                continue;

            auto actions = tool_actions(grp.second, tool_actions_grp, to_activate, event);
            QIcon icon = QIcon::fromTheme(icons[grp.first]);
            QMenu* menu = action_menu(icon, "", layout_tools);
            for ( auto action: actions )
                menu->addAction(action);
        }
        switch_tool(to_activate);


        // Views
        view_actions = new QActionGroup(parent);
        view_actions->setExclusive(true);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        view_actions->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
#endif

        toolbar_spacers.push_back(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        layout_tools->addItem(toolbar_spacers.back());

        layout_tools->addWidget(action_button_exclusive_opt(view_action(
            QIcon::fromTheme("player-time"), tr("Timeline"),
            view_actions, ui.time_container, true
        )));

        layout_tools->addWidget(action_button_exclusive_opt(view_action(
            QIcon::fromTheme("fill-color"), tr("Fill Style"),
            view_actions, ui.fill_style_widget
        )));

        layout_tools->addWidget(action_button_exclusive_opt(view_action(
            QIcon::fromTheme("object-stroke-style"), tr("Stroke Style"),
            view_actions, ui.stroke_style_widget
        )));
    }

    void selection_move(command::ReorderCommand::SpecialPosition pos, const QString& msg)
    {
        auto sel = scene.cleaned_selection();
        if ( sel.empty() )
            return;

        command::UndoMacroGuard guard(msg, current_document.get(), false);

        for ( const auto& node : sel )
        {
            auto shape = node->cast<model::ShapeElement>();
            if ( !shape )
                continue;

            int position = pos;
            if ( !command::ReorderCommand::resolve_position(shape, position) )
                continue;

            guard.start();
            node->push_command(new command::ReorderCommand(shape, pos));
        }
    }

    void selection_raise()
    {
        selection_move(command::ReorderCommand::MoveUp, tr("Raise"));
    }

    void selection_lower()
    {
        selection_move(command::ReorderCommand::MoveDown, tr("Lower"));
    }

    void init_toolbar_actions()
    {
        // Clipboard
        layout_actions->addWidget(action_button(
            document_action_public(QIcon::fromTheme("edit-cut"), tr("Cut"), &MainWindow::cut)
        ));
        layout_actions->addWidget(action_button(
            document_action_public(QIcon::fromTheme("edit-copy"), tr("Copy"), &MainWindow::copy)
        ));
        layout_actions->addWidget(action_button(
            document_action_public(QIcon::fromTheme("edit-paste"), tr("Paste"), &MainWindow::paste)
        ));

        layout_actions->addWidget(action_button(
            document_action_public(QIcon::fromTheme("edit-delete"), tr("Delete Selected"), &MainWindow::delete_shapes)
        ));

        toolbar_spacers.push_back(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        layout_actions->addItem(toolbar_spacers.back());

        // Layer
        layout_actions->addWidget(action_button(
            document_action(QIcon::fromTheme("layer-raise"), tr("Raise Above"), &Private::selection_raise)
        ));

        layout_actions->addWidget(action_button(
            document_action(QIcon::fromTheme("layer-lower"), tr("Lower Below"), &Private::selection_lower)
        ));

        // Undo-redo
        action_undo = new QAction(QIcon::fromTheme("edit-undo"), tr("Undo"), parent);
        layout_actions->addWidget(action_button(action_undo));
        action_redo = new QAction(QIcon::fromTheme("edit-redo"), tr("Redo"), parent);
        layout_actions->addWidget(action_button(action_redo));
    }

    void init_toolbar_edit()
    {
        layout_edit_actions = new QHBoxLayout();
        layout_edit_actions->setMargin(0);
        layout_edit_actions->setSpacing(0);
        ui.widget_edit_actions->setLayout(layout_edit_actions);


        // Document actions
        layout_edit_actions->addWidget(action_button(
            document_action(QIcon::fromTheme("document-new"), tr("New"), &Private::document_new)
        ));

        QMenu* menu_open = action_menu(QIcon::fromTheme("document-open"), tr("Open..."), layout_edit_actions);
        menu_open->addAction(
            document_action(QIcon::fromTheme("document-open"), tr("Open"), &Private::document_open)
        );
        menu_open->addAction(
            document_action(QIcon::fromTheme("document-import"), tr("Import as Composition"), &Private::document_import)
        );

        QMenu* menu_save = action_menu(QIcon::fromTheme("document-save"), tr("Save..."), layout_edit_actions);
        menu_save->addAction(
            document_action(QIcon::fromTheme("document-save"), tr("Save"), &Private::document_save)
        );
        menu_save->addAction(
            document_action(QIcon::fromTheme("document-save-as"), tr("Save As"), &Private::document_save_as)
        );
        menu_save->addAction(
            document_action(QIcon::fromTheme("document-export"), tr("Export"), &Private::document_export)
        );
        menu_save->addAction(
            document_action(QIcon::fromTheme("view-preview"), tr("Save Frame as PNG"), &Private::document_frame_to_png)
        );

        layout_edit_actions->addWidget(action_button(
            document_action(QIcon::fromTheme("document-send"), tr("Send to Telegram"), &Private::document_export_telegram)
        ));

        // Spacer
        layout_edit_actions->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

        // Views
        layer_view = new gui::LayerView(parent);
        layer_view->setIconSize({80, 80});
        ui.gridLayout->addWidget(layer_view, 1, 2, 2, 1);
        layer_view->set_base_model(&document_node_model);
        layout_edit_actions->addWidget(action_button_exclusive_opt(view_action(
            QIcon::fromTheme("dialog-layers"), tr("Layers"),
            view_actions, layer_view
        )));
        ScrollAreaEventFilter::setup_scroller(layer_view);

        layout_edit_actions->addWidget(action_button_exclusive_opt(view_action(
            QIcon::fromTheme("document-properties"), tr("Advanced Properties"),
            view_actions, ui.property_widget
        )));
        layer_view->setMinimumWidth(512);

        auto help = new QAction(QIcon::fromTheme("question"), tr("Help"), parent);
        layout_edit_actions->addWidget(action_button(help));
        connect(help, &QAction::triggered, parent, [this]{
            HelpDialog(parent).exec();
        });

        /*
        // Toggler
        layout_tools->addWidget(action_button(view_action(
            QIcon::fromTheme("overflow-menu"), tr("More Tools"),
            nullptr, ui.widget_actions, true
        )));
        */
    }

    QAction* document_action(const QIcon& icon, const QString& text, void (Private::* func)())
    {
        QAction* action = new QAction(icon, text, parent);
        connect(action, &QAction::triggered, parent, [this, func]{
            (this->*func)();
        });
        return action;
    }

    template<class Callback>
    QAction* document_action_public(const QIcon& icon, const QString& text, Callback func)
    {
        QAction* action = new QAction(icon, text, parent);
        connect(action, &QAction::triggered, parent, [this, func]{
            (parent->*func)();
        });
        return action;
    }

    QAction* view_action(const QIcon& icon, const QString& text, QActionGroup* group,
                         QWidget* target, bool checked = false)
    {
        QAction* action = new QAction(icon, text, parent);
        action->setCheckable(true);
        action->setChecked(checked);
        target->setVisible(checked);
        action->setActionGroup(group);
        connect(action, &QAction::toggled, target, &QWidget::setVisible);

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

            gui::ImportExportDialog dialog(opts, parent);

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
                gui::ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
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
            gui::ImportExportDialog dialog(current_document->io_options(), ui.centralwidget->parentWidget());
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
        exporting_png = false;
        save_document(true, true);
    }

    bool exporting_png = false;
    void document_frame_to_png()
    {
        exporting_png = true;
        if ( !file_picker.select_save(tr("Frame %1.png").arg(current_document->current_time()), true, "image/png") )
        {
            exporting_png = false;
        }
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
        QBoxLayout::Direction toolbar_direction;

        QSize screen_size = QApplication::primaryScreen()->size();

#ifdef Q_OS_ANDROID_FAKE
        screen_size = parent->size();
#endif

        std::pair<QSizePolicy::Policy, QSizePolicy::Policy> spacer_policy;

        if ( screen_size.width() > screen_size.height() )
        {
            toolbar_orientation = Qt::Vertical;
            toolbar_direction = QBoxLayout::TopToBottom;
            mins = screen_size.height() * 0.7;
            spacer_policy = {QSizePolicy::Minimum, QSizePolicy::Expanding};
        }
        else
        {
            spacer_policy = {QSizePolicy::Expanding, QSizePolicy::Minimum};
            toolbar_orientation = Qt::Horizontal;
            toolbar_direction = QBoxLayout::LeftToRight;
            mins = screen_size.width();
        }

        for ( const auto& spacer : toolbar_spacers )
            spacer->changeSize(0, 0, spacer_policy.first, spacer_policy.second);


        int button_w = qFloor(mins / 9.);
        QSize button_size(button_w, button_w);

        for ( QToolButton* btn : parent->findChildren<QToolButton*>() )
            btn->setIconSize(button_size);

        timeline_slider->setFixedHeight(button_w);
        timeline_slider->set_slider_size(screen_size.width() / 10);

        ui.widget_tools_container_side->setVisible(false);
        ui.widget_tools_container_bottom->setVisible(false);

        tool_layout_extent = button_w;

        layout_actions->setDirection(toolbar_direction);
        layout_tools->setDirection(toolbar_direction);
        QSize tool_layout_size(tool_layout_extent, tool_layout_extent);
//        layout_tools->set_fixed_item_size(tool_layout_size);
//        layout_actions->set_fixed_item_size(tool_layout_size);

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

        if ( export_opts && exporting_png )
        {
            QImage pix(current_document->size(), QImage::Format_RGBA8888);
            pix.fill(Qt::transparent);
            QPainter painter(&pix);
            painter.setRenderHint(QPainter::Antialiasing);
            current_document->main()->paint(&painter, current_document->current_time(), model::VisualNode::Render);
            painter.end();
            QByteArray data;
            QBuffer buf(&data);
            buf.open(QIODevice::WriteOnly);
            pix.save(&buf, "PNG");
            buf.close();
            AndroidFilePicker::write_content_uri(url, data);
            exporting_png = false;
            return false;
        }

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

    void add_property_widgets_object(model::Object* node, BetterToolboxWidget* toolbox)
    {
        using Traits = model::PropertyTraits;
        std::vector<std::pair<QWidget*, model::BaseProperty*>> props;

        for ( const auto& prop : node->properties() )
        {
            auto traits = prop->traits();
            if ( traits.type == Traits::ObjectReference || (traits.flags & (Traits::List|Traits::Hidden)) )
            {
                continue;
            }
            else if ( traits.type == Traits::Object )
            {
                add_property_widgets_object(prop->value().value<model::Object*>(), toolbox);
            }
            else if ( !(traits.flags & Traits::Visual)  )
            {
                continue;
            }
            else if ( auto wid = property_delegate.editor_from_property(prop, nullptr) )
            {
                QWidget* prop_widget = new QWidget();
                QVBoxLayout* lay = new QVBoxLayout();
                prop_widget->setLayout(lay);
                lay->addWidget(wid);
                lay->setMargin(0);
                lay->setSpacing(0);
                wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

                if ( traits.flags & Traits::Animated )
                {
                    auto anim = static_cast<model::AnimatableBase*>(prop);
                    QHBoxLayout* btnlay = new QHBoxLayout();

                    auto btn_add_kf = new QToolButton();
                    btn_add_kf->setIcon(
                        QIcon(gui::GlaxnimateApp::instance()->data_file("images/icons/keyframe-add.svg"))
                    );
                    btn_add_kf->setText(tr("Add keyframe"));
                    connect(btn_add_kf, &QToolButton::clicked, node, [anim]{
                        anim->object()->push_command(
                            new command::SetKeyframe(anim, anim->time(), anim->value(), true)
                        );
                    });
                    btn_add_kf->setIconSize(QSize(80, 80));
                    btnlay->addWidget(btn_add_kf);

                    auto btn_rm_kf = new QToolButton();
                    btn_rm_kf->setIcon(
                        QIcon(gui::GlaxnimateApp::instance()->data_file("images/icons/keyframe-remove.svg"))
                    );
                    btn_rm_kf->setText(tr("Remove keyframe"));
                    connect(btn_rm_kf, &QToolButton::clicked, node, [anim]{
                        if ( anim->has_keyframe(anim->time()) )
                        {
                            anim->object()->push_command(
                                new command::RemoveKeyframeTime(anim, anim->time())
                            );
                        }
                    });
                    btn_rm_kf->setIconSize(QSize(80, 80));
                    btnlay->addWidget(btn_rm_kf);

                    lay->addLayout(btnlay);
                }

                props.emplace_back(wid, prop);
                property_delegate.set_editor_data(wid, prop);
                toolbox->addItem(prop_widget, prop->name());
            }
        }
        connect(node, &model::Object::visual_property_changed, props[0].first, [props, this]{
            for ( const auto& p : props )
                property_delegate.set_editor_data(p.first, p.second);
        });

    }

    void add_property_widgets(model::VisualNode* node, BetterToolboxWidget* toolbox)
    {
        BetterToolboxWidget* sub_toolbox = new BetterToolboxWidget();
        int index  = toolbox->count();
        toolbox->addItem(sub_toolbox, node->tree_icon(), node->object_name());

        connect(node, &model::DocumentNode::name_changed, sub_toolbox,
            [toolbox, index](const QString& name){
                toolbox->setItemText(index, name);
            }
        );
        add_property_widgets_object(node, sub_toolbox);

        if ( auto grp = node->cast<model::Group>() )
        {
            for ( const auto& child : grp->shapes )
            {
                if ( !child->is_instance<model::Group>() )
                    add_property_widgets(child.get(), toolbox);
            }
        }
    }


    void set_property_widgets(model::VisualNode* node)
    {
        clear_property_widgets();
        if ( node )
        {
            BetterToolboxWidget* toolbox = new BetterToolboxWidget();
            toolbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            ui.property_widget_layout->addWidget(toolbox);
            add_property_widgets(node, toolbox);
            qDebug() << toolbox->sizeHint().width();
            ui.property_widget->setMinimumWidth(toolbox->sizeHint().width() + 6);
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

            for ( const auto& grp : gui::tools::Registry::instance() )
            {
                for ( const auto& tool : grp.second )
                {
                    tool.second->retranslate();
                }
            }
            break;
        default:
            break;
    }
}

glaxnimate::model::Document* MainWindow::document() const
{
    return d->current_document.get();
}


glaxnimate::model::Composition* MainWindow::current_composition() const
{
    return d->comp;
}

glaxnimate::model::VisualNode* MainWindow::current_document_node() const
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

glaxnimate::model::BrushStyle* MainWindow::linked_brush_style(bool secondary) const
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

    if ( sender() != d->layer_view )
        d->layer_view->set_current_node(node);
}

void MainWindow::set_current_composition(model::Composition* comp)
{
    d->comp = comp ? comp : d->current_document->main();
    d->scene.set_composition(comp);
    d->layer_view->set_composition(comp);
}

void MainWindow::switch_tool(gui::tools::Tool* tool)
{
    d->switch_tool(tool);
}

std::vector<glaxnimate::model::VisualNode*> MainWindow::cleaned_selection() const
{
    return d->scene.cleaned_selection();
}

std::vector<glaxnimate::io::mime::MimeSerializer *> MainWindow::supported_mimes() const
{
    return {
        io::IoRegistry::instance().serializer_from_slug("glaxnimate")
    };
}

void MainWindow::tool_triggered(bool checked)
{
    if ( checked )
        d->switch_tool(static_cast<QAction*>(sender())->data().value<gui::tools::Tool*>());
}

void MainWindow::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);

#ifdef Q_OS_ANDROID_FAKE
    d->adjust_size();
#endif
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    d->adjust_size();
}

void MainWindow::set_selection(const std::vector<model::VisualNode*>& selected)
{
    d->scene.user_select(selected, gui::graphics::DocumentScene::Replace);
}

void MainWindow::update_selection(const std::vector<model::VisualNode *> &selected, const std::vector<model::VisualNode *> &deselected)
{
    if ( d->updating_selection )
        return;

    std::unique_lock lock(d->updating_selection);

    if ( sender() != &d->scene )
    {
        d->scene.user_select(deselected, gui::graphics::DocumentScene::Remove);
        d->scene.user_select(selected, gui::graphics::DocumentScene::Append);
    }

    if ( sender() != d->layer_view )
        d->layer_view->update_selection(selected, deselected);
}

void MainWindow::open_intent(const QUrl &uri)
{
    if ( d->close_document() )
    {
        d->open_url(uri, false);
    }
}
