#include "glaxnimate_window_p.hpp"

#include "app/application.hpp"

#include "tools/base.hpp"
#include "model/shapes/group.hpp"

#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/dialogs/about_dialog.hpp"
#include "widgets/dialogs/resize_dialog.hpp"
#include "widgets/view_transform_widget.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/node_menu.hpp"
#include "style/better_elide_delegate.hpp"
#include "glaxnimate_app.hpp"

void GlaxnimateWindow::Private::setupUi(bool restore_state, GlaxnimateWindow* parent)
{
    this->parent = parent;
    ui.setupUi(parent);
    redo_text = ui.action_redo->text();
    undo_text = ui.action_undo->text();

    // Standard Shorcuts
    ui.action_new->setShortcut(QKeySequence::New);
    ui.action_open->setShortcut(QKeySequence::Open);
    ui.action_close->setShortcut(QKeySequence::Close);
    ui.action_reload->setShortcut(QKeySequence("Ctrl+F5", QKeySequence::PortableText));
    ui.action_save->setShortcut(QKeySequence::Save);
    ui.action_save_as->setShortcut(QKeySequence::SaveAs);
    ui.action_quit->setShortcut(QKeySequence::Quit);
    ui.action_copy->setShortcut(QKeySequence::Copy);
    ui.action_cut->setShortcut(QKeySequence::Cut);
    ui.action_paste->setShortcut(QKeySequence::Paste);
    ui.action_select_all->setShortcut(QKeySequence::SelectAll);
    ui.action_undo->setShortcut(QKeySequence::Undo);
    ui.action_redo->setShortcut(QKeySequence::Redo);
    ui.action_group->setShortcut(QKeySequence("Ctrl+G", QKeySequence::PortableText));
    ui.action_ungroup->setShortcut(QKeySequence("Ctrl+Shift+G", QKeySequence::PortableText));

    // Actions
    connect(ui.action_copy, &QAction::triggered, parent, &GlaxnimateWindow::copy);
    connect(ui.action_paste, &QAction::triggered, parent, &GlaxnimateWindow::paste);
    connect(ui.action_cut, &QAction::triggered, parent, &GlaxnimateWindow::cut);
    connect(ui.action_reload, &QAction::triggered, parent, &GlaxnimateWindow::document_reload);
    connect(ui.action_raise_to_top, &QAction::triggered, parent, &GlaxnimateWindow::layer_top);
    connect(ui.action_raise, &QAction::triggered, parent, &GlaxnimateWindow::layer_raise);
    connect(ui.action_lower, &QAction::triggered, parent, &GlaxnimateWindow::layer_lower);
    connect(ui.action_lower_to_bottom, &QAction::triggered, parent, &GlaxnimateWindow::layer_bottom);
    connect(ui.action_group, &QAction::triggered, parent, &GlaxnimateWindow::group_shapes);
    connect(ui.action_ungroup, &QAction::triggered, parent, &GlaxnimateWindow::ungroup_shapes);
    connect(ui.action_quit, &QAction::triggered, parent, &GlaxnimateWindow::close);
    connect(ui.action_move_to, &QAction::triggered, parent, &GlaxnimateWindow::move_to);
    connect(ui.action_validate_tgs, &QAction::triggered, parent, &GlaxnimateWindow::validate_tgs);
    connect(ui.action_resize, &QAction::triggered, parent, [this]{ ResizeDialog(this->parent).resize_document(current_document.get()); });
    connect(ui.action_donate, &QAction::triggered, parent, &GlaxnimateWindow::help_donate);

    // Menu Views
    for ( QDockWidget* wid : parent->findChildren<QDockWidget*>() )
    {
        QAction* act = wid->toggleViewAction();
        act->setIcon(wid->windowIcon());
        ui.menu_views->addAction(act);
        wid->setStyle(&dock_style);
    }

    // Tool Actions
    QActionGroup *tool_actions = new QActionGroup(parent);
    tool_actions->setExclusive(true);

    dock_tools_layout = new FlowLayout();
    ui.dock_tools_layout_parent->insertLayout(0, dock_tools_layout);
    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            QAction* action = tool.second->get_action();
            action->setParent(parent);
            ui.menu_tools->addAction(action);
            action->setActionGroup(tool_actions);
            ScalableButton *button = tool.second->get_button();
            connect(action, &QAction::triggered, parent, &GlaxnimateWindow::tool_triggered);

            button->resize(16, 16);
            dock_tools_layout->addWidget(button);

            ui.tool_settings_widget->addWidget(tool.second->get_settings_widget());

            tool.second->retranslate();

            if ( !active_tool )
            {
                switch_tool(tool.second.get());
                action->setChecked(true);
            }
        }
        ui.menu_tools->addSeparator();
    }

    // Item views
    ui.view_document_node->setModel(&document_node_model);
    ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    ui.view_document_node->setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &color_delegate);
    ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnLocked);
    QObject::connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::currentChanged,
                        parent, &GlaxnimateWindow::document_treeview_current_changed);

    ui.view_document_node->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui.view_document_node, &QWidget::customContextMenuRequested, parent,
        [this](const QPoint& pos){
            auto index = ui.view_document_node->indexAt(pos);
            if ( auto node = document_node_model.node(index) )
                NodeMenu(node, this->parent, this->parent).exec(ui.view_document_node->mapToGlobal(pos));
        }
    );

    ui.view_properties->setModel(&property_model);
    ui.view_properties->setItemDelegateForColumn(1, &property_delegate);
    ui.view_properties->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.view_properties->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::selectionChanged, parent, &GlaxnimateWindow::document_treeview_selection_changed);

    // Tool buttons
    ui.btn_layer_add->setMenu(ui.menu_new_layer);

    // Time spinner

    // Transform Widget
    view_trans_widget = new ViewTransformWidget(ui.status_bar);
    ui.status_bar->addPermanentWidget(view_trans_widget);
    connect(view_trans_widget, &ViewTransformWidget::zoom_changed, ui.graphics_view, &GlaxnimateGraphicsView::set_zoom);
    connect(ui.graphics_view, &GlaxnimateGraphicsView::zoomed, view_trans_widget, &ViewTransformWidget::set_zoom);
    connect(view_trans_widget, &ViewTransformWidget::zoom_in, ui.graphics_view, &GlaxnimateGraphicsView::zoom_in);
    connect(view_trans_widget, &ViewTransformWidget::zoom_out, ui.graphics_view, &GlaxnimateGraphicsView::zoom_out);
    connect(view_trans_widget, &ViewTransformWidget::angle_changed, ui.graphics_view, &GlaxnimateGraphicsView::set_rotation);
    connect(ui.graphics_view, &GlaxnimateGraphicsView::rotated, view_trans_widget, &ViewTransformWidget::set_angle);
    connect(view_trans_widget, &ViewTransformWidget::view_fit, parent, &GlaxnimateWindow::view_fit);

    // Graphics scene
    ui.graphics_view->setScene(&scene);
    ui.graphics_view->set_tool_target(parent);
    connect(&scene, &graphics::DocumentScene::node_user_selected, parent, &GlaxnimateWindow::scene_selection_changed);

    // dialogs
    dialog_import_status = new IoStatusDialog(QIcon::fromTheme("document-open"), tr("Open File"), false, parent);
    dialog_export_status = new IoStatusDialog(QIcon::fromTheme("document-save"), tr("Save File"), false, parent);
    about_dialog = new AboutDialog(parent);

    // Recent files
    recent_files = app::settings::get<QStringList>("open_save", "recent_files");
    reload_recent_menu();
    connect(ui.menu_open_recent, &QMenu::triggered, parent, &GlaxnimateWindow::document_open_recent);

    // Scripting
    ui.console_input->setHistory(app::settings::get<QStringList>("scripting", "history"));

    for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
    {
        ui.console_language->addItem(engine->label());
        if ( engine->slug() == "python" )
            ui.console_language->setCurrentIndex(ui.console_language->count()-1);
    }

    // Plugins
    auto& par = app::scripting::PluginActionRegistry::instance();
    for ( auto act : par.enabled() )
    {
        ui.menu_plugins->addAction(par.make_qaction(act));
    }
    connect(&par, &app::scripting::PluginActionRegistry::action_added, parent, [this](app::scripting::ActionService* action) {
        ui.menu_plugins->addAction(app::scripting::PluginActionRegistry::instance().make_qaction(action));
    });
    connect(
        &app::scripting::PluginRegistry::instance(),
        &app::scripting::PluginRegistry::script_needs_running,
        parent,
        &GlaxnimateWindow::script_needs_running
    );
    connect(
        &app::scripting::PluginRegistry::instance(),
        &app::scripting::PluginRegistry::loaded,
        parent,
        &GlaxnimateWindow::script_reloaded
    );

    // Logs
    log_model.populate(GlaxnimateApp::instance()->log_lines());
    ui.view_logs->setModel(&log_model);
    ui.view_logs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.view_logs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
//     ui.view_logs->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui.view_logs->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    auto del = new style::BetterElideDelegate(Qt::ElideLeft, ui.view_logs);
    ui.view_logs->setItemDelegateForColumn(2, del);

    // Swatches

    palette_model.setSearchPaths(app::Application::instance()->data_paths_unchecked("palettes"));
    palette_model.setSavePath(app::Application::instance()->writable_data_path("palettes"));
    palette_model.load();
    ui.color_selector->set_palette_model(&palette_model);
    ui.stroke_style_widget->set_palette_model(&palette_model);
    ui.document_swatch_widget->set_palette_model(&palette_model);

    connect(ui.document_swatch_widget, &DocumentSwatchWidget::needs_new_color, [this]{
        ui.document_swatch_widget->add_new_color(ui.color_selector->current_color());
    });
    connect(ui.document_swatch_widget, &DocumentSwatchWidget::current_color_def, [this](model::BrushStyle* sty){
        set_color_def_primary(sty);
    });
    connect(ui.document_swatch_widget, &DocumentSwatchWidget::secondary_color_def, [this](model::BrushStyle* sty){
        set_color_def_secondary(sty);
    });

    // Arrange docks
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_layers);

    parent->tabifyDockWidget(ui.dock_timeline, ui.dock_properties);
    parent->tabifyDockWidget(ui.dock_properties, ui.dock_script_console);
    parent->tabifyDockWidget(ui.dock_script_console, ui.dock_logs);
    ui.dock_timeline->raise();

    parent->tabifyDockWidget(ui.dock_colors, ui.dock_stroke);
    parent->tabifyDockWidget(ui.dock_stroke, ui.dock_swatches);
    parent->tabifyDockWidget(ui.dock_swatches, ui.dock_undo);
    ui.dock_colors->raise();
    ui.dock_swatches->setVisible(false);

    parent->resizeDocks(
        {ui.dock_layers},
        {1},
        Qt::Horizontal
    );
    parent->resizeDocks({ui.dock_tools}, {200}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_timeline}, {parent->height()/3}, Qt::Vertical);
    ui.dock_script_console->setVisible(false);
    ui.dock_logs->setVisible(false);

    // Auto Screenshots for docs
#if 0
    QDir("/tmp/").mkpath("menus");
    for ( auto widget : parent->findChildren<QMenu*>() )
    {
        widget->show();
        QString name = "/tmp/menus/" + widget->objectName().mid(5);
        QPixmap pic(widget->size());
        widget->render(&pic);
        name += ".png";
        pic.save(name);
    }
#endif

    // Restore state
    // NOTE: keep at the end so we do this once all the widgets are in their default spots
    if ( restore_state )
    {
        parent->restoreGeometry(app::settings::get<QByteArray>("ui", "window_geometry"));
        parent->restoreState(app::settings::get<QByteArray>("ui", "window_state"));
    }
}

void GlaxnimateWindow::Private::retranslateUi(QMainWindow* parent)
{
    ui.retranslateUi(parent);
    redo_text = ui.action_redo->text();
    undo_text = ui.action_undo->text();

    ui.action_undo->setText(redo_text.arg(current_document->undo_stack().undoText()));
    ui.action_redo->setText(redo_text.arg(current_document->undo_stack().redoText()));

    for ( const auto& grp : tools::Registry::instance() )
        for ( const auto& tool : grp.second )
            tool.second->retranslate();
}

void GlaxnimateWindow::Private::document_treeview_current_changed(const QModelIndex& index)
{
    model::Stroke* stroke = nullptr;
    model::Fill* fill = nullptr;
    if ( auto node = document_node_model.node(index) )
    {
        property_model.set_object(node);
        ui.timeline_widget->set_active(node);
        ui.view_properties->expandAll();

        stroke = qobject_cast<model::Stroke*>(node);
        fill = qobject_cast<model::Fill*>(node);
        if ( !stroke && !fill )
        {
            auto group = qobject_cast<model::Group*>(node);

            if ( !group )
            {
                if ( auto parent = node->docnode_parent() )
                    group = qobject_cast<model::Group*>(parent);
            }

            if ( group )
            {
                int stroke_count = 0;
                int fill_count = 0;
                for ( const auto& shape : group->shapes )
                {
                    if ( auto s = qobject_cast<model::Stroke*>(shape.get()) )
                    {
                        stroke = s;
                        stroke_count++;
                    }
                    else if ( auto f = qobject_cast<model::Fill*>(shape.get()) )
                    {
                        fill = f;
                        fill_count++;
                    }
                }

                if ( stroke_count > 1 )
                    stroke = nullptr;

                if ( fill_count > 1 )
                    fill = nullptr;
            }
        }
    }

    ui.stroke_style_widget->set_shape(stroke);
    /// \todo rename to fill_style_widget
    ui.color_selector->set_shape(fill);
}

void GlaxnimateWindow::Private::view_fit()
{
    ui.graphics_view->view_fit();
}

void GlaxnimateWindow::Private::reload_recent_menu()
{
    ui.menu_open_recent->clear();
    for ( const auto& recent : recent_files )
    {
        QAction* act = new QAction(QIcon::fromTheme("video-x-generic"), QFileInfo(recent).fileName(), ui.menu_open_recent);
        act->setToolTip(recent);
        act->setData(recent);
        ui.menu_open_recent->addAction(act);
    }
}

void GlaxnimateWindow::Private::most_recent_file(const QString& s)
{
    recent_files.removeAll(s);
    recent_files.push_front(s);

    int max = app::settings::get<int>("open_save", "max_recent_files");
    if ( recent_files.size() > max )
        recent_files.erase(recent_files.begin() + max, recent_files.end());

    reload_recent_menu();
}

void GlaxnimateWindow::Private::show_warning(const QString& title, const QString& message, app::log::Severity icon)
{
    ui.message_widget->queue_message({message, icon});
    app::log::Log(title).log(message, icon);
}

void GlaxnimateWindow::Private::help_about()
{
    about_dialog->show();
}

void GlaxnimateWindow::Private::shutdown()
{
    app::settings::set("ui", "window_geometry", parent->saveGeometry());
    app::settings::set("ui", "window_state", parent->saveState());
    app::settings::set("open_save", "recent_files", recent_files);

    ui.color_selector->save_settings();
    ui.stroke_style_widget->save_settings();

    QStringList history = ui.console_input->history();
    int max_history = app::settings::get<int>("scripting", "max_history");
    if ( history.size() > max_history )
        history.erase(history.begin(), history.end() - max_history);
    app::settings::set("scripting", "history", history);
    script_contexts.clear();
}

void GlaxnimateWindow::Private::document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    for ( const auto& index : selected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = document_node_model.node(index) )
                scene.add_selection(node);

    for ( const auto& index : deselected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = document_node_model.node(index) )
                scene.remove_selection(node);
}

void GlaxnimateWindow::Private::scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected)
{
    for ( model::DocumentNode* node : deselected )
    {
        ui.view_document_node->selectionModel()->select(
            document_node_model.node_index(node),
            QItemSelectionModel::Deselect|QItemSelectionModel::Rows
        );
    }

    for ( model::DocumentNode* node : selected )
    {
        ui.view_document_node->selectionModel()->select(
            document_node_model.node_index(node),
            QItemSelectionModel::Select|QItemSelectionModel::Rows
        );
    }

    if ( !selected.empty() )
    {
        ui.view_document_node->selectionModel()->setCurrentIndex(
            document_node_model.node_index(selected.back()),
            QItemSelectionModel::NoUpdate
        );
    }
    else
    {
        auto current = ui.view_document_node->currentIndex();
        if ( current.isValid() && ! ui.view_document_node->selectionModel()->isSelected(current) )
            ui.view_document_node->setCurrentIndex(QModelIndex{});
    }
}

void GlaxnimateWindow::Private::switch_tool(tools::Tool* tool)
{
    if ( !tool )
        return;

    active_tool = tool;
    ui.graphics_view->set_active_tool(tool);
    scene.set_active_tool(tool);
    ui.tool_settings_widget->setCurrentWidget(tool->get_settings_widget());
}

void GlaxnimateWindow::Private::switch_tool_action(QAction* action)
{
    switch_tool(action->data().value<tools::Tool*>());
}

void GlaxnimateWindow::Private::status_message(const QString& message, int duration)
{
    ui.status_bar->showMessage(message, duration);
}

void GlaxnimateWindow::Private::set_color_def_primary(model::BrushStyle* sty)
{
    if ( auto shape = ui.color_selector->shape() )
        shape->use.set_undoable(QVariant::fromValue(sty));
}

void GlaxnimateWindow::Private::set_color_def_secondary(model::BrushStyle* sty)
{
    if ( auto shape = ui.stroke_style_widget->shape() )
        shape->use.set_undoable(QVariant::fromValue(sty));
}
