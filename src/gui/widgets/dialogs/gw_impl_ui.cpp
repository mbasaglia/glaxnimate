#include "glaxnimate_window_p.hpp"

#include <QComboBox>

#include "app/settings/keyboard_shortcuts.hpp"

#include "tools/base.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/image.hpp"

#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/dialogs/about_dialog.hpp"
#include "widgets/dialogs/resize_dialog.hpp"
#include "widgets/dialogs/timing_dialog.hpp"
#include "widgets/dialogs/document_metadata_dialog.hpp"
#include "widgets/dialogs/trace_dialog.hpp"

#include "widgets/view_transform_widget.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/node_menu.hpp"

#include "style/better_elide_delegate.hpp"
#include "tools/edit_tool.hpp"
#include "plugin/action.hpp"
#include "glaxnimate_app.hpp"

static QToolButton* action_button(QAction* action, QWidget* parent)
{
    auto button = new ScalableButton(parent);
    button->setDefaultAction(action);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return button;
}

static void action_combo(QComboBox* box, QAction* action)
{
    int index = box->count();
    box->addItem(action->icon(), action->text(), QVariant::fromValue(action));
    QObject::connect(action, &QAction::triggered, box, [index, box]{
        box->setCurrentIndex(index);
    });
    QObject::connect(action, &QAction::changed, box, [index, box, action]{
        box->setItemIcon(index, action->icon());
        box->setItemText(index, action->text());
    });
}

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
    ui.action_paste_as_composition->setShortcut(QKeySequence("Ctrl+Shift+V", QKeySequence::PortableText));
    ui.action_select_all->setShortcut(QKeySequence::SelectAll);
    ui.action_undo->setShortcut(QKeySequence::Undo);
    ui.action_redo->setShortcut(QKeySequence::Redo);
    ui.action_group->setShortcut(QKeySequence("Ctrl+G", QKeySequence::PortableText));
    ui.action_ungroup->setShortcut(QKeySequence("Ctrl+Shift+G", QKeySequence::PortableText));
    ui.action_open_last->setShortcut(QKeySequence("Ctrl+Shift+O", QKeySequence::PortableText));
    ui.action_import_image->setShortcut(QKeySequence("Ctrl+I", QKeySequence::PortableText));
    ui.action_import->setShortcut(QKeySequence("Ctrl+Shift+I", QKeySequence::PortableText));
    ui.action_node_remove->setShortcut(QKeySequence("Del", QKeySequence::PortableText));
    ui.action_delete->setShortcut(QKeySequence("Del", QKeySequence::PortableText));
    ui.action_export->setShortcut(QKeySequence("Ctrl+E", QKeySequence::PortableText));
    ui.action_export_as->setShortcut(QKeySequence("Ctrl+Shift+E", QKeySequence::PortableText));
    ui.action_frame_prev->setShortcut(QKeySequence("Left", QKeySequence::PortableText));
    ui.action_frame_next->setShortcut(QKeySequence("Right", QKeySequence::PortableText));
    ui.action_duplicate->setShortcut(QKeySequence("Ctrl+D", QKeySequence::PortableText));

    // Actions
    connect(ui.action_copy, &QAction::triggered, parent, &GlaxnimateWindow::copy);
    connect(ui.action_paste, &QAction::triggered, parent, &GlaxnimateWindow::paste);
    connect(ui.action_paste_as_composition, &QAction::triggered, parent, [this]{paste(true);});
    connect(ui.action_cut, &QAction::triggered, parent, &GlaxnimateWindow::cut);
    connect(ui.action_duplicate, &QAction::triggered, parent, &GlaxnimateWindow::duplicate_selection);
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
    connect(ui.action_new_layer, &QAction::triggered, parent, [this]{layer_new_layer();});
    connect(ui.action_new_group, &QAction::triggered, parent, [this]{layer_new_group();});
    connect(ui.action_new_fill, &QAction::triggered, parent, [this]{layer_new_fill();});
    connect(ui.action_new_stroke, &QAction::triggered, parent, [this]{layer_new_stroke();});
    connect(ui.action_open_last, &QAction::triggered, parent, [this]{
        if ( !recent_files.isEmpty() )
            document_open_from_filename(recent_files[0]);
    });
    connect(ui.action_import_image, &QAction::triggered, parent, [this]{import_image();});
    connect(ui.action_delete, &QAction::triggered, parent, &GlaxnimateWindow::delete_selected);
    connect(ui.action_export, &QAction::triggered, parent, &GlaxnimateWindow::document_export);
    connect(ui.action_export_as, &QAction::triggered, parent, &GlaxnimateWindow::document_export_as);
    connect(ui.action_document_cleanup, &QAction::triggered, parent, [this]{cleanup_document();});
    connect(ui.action_timing, &QAction::triggered, parent, [this]{
        TimingDialog(current_document.get(), this->parent).exec();
    });
    connect(ui.action_play, &QAction::triggered, ui.play_controls, &FrameControlsWidget::toggle_play);
    connect(ui.play_controls, &FrameControlsWidget::play_started, parent, [this]{
        ui.action_play->setText(tr("Pause"));
        ui.action_play->setIcon(QIcon::fromTheme("media-playback-pause"));
    });
    connect(ui.play_controls, &FrameControlsWidget::play_stopped, parent, [this]{
        ui.action_play->setText(tr("Play"));
        ui.action_play->setIcon(QIcon::fromTheme("media-playback-start"));
    });
    connect(ui.play_controls, &FrameControlsWidget::record_toggled, ui.action_record, &QAction::setChecked);
    connect(ui.action_record, &QAction::triggered, ui.play_controls, &FrameControlsWidget::set_record_enabled);
    connect(ui.action_frame_first, &QAction::triggered, ui.play_controls, &FrameControlsWidget::go_first);
    connect(ui.action_frame_prev, &QAction::triggered, ui.play_controls, &FrameControlsWidget::go_prev);
    connect(ui.action_frame_next, &QAction::triggered, ui.play_controls, &FrameControlsWidget::go_next);
    connect(ui.action_frame_last, &QAction::triggered, ui.play_controls, &FrameControlsWidget::go_last);
    connect(ui.play_controls, &FrameControlsWidget::loop_changed, ui.action_play_loop, &QAction::setChecked);
    connect(ui.action_play_loop, &QAction::triggered, ui.play_controls, &FrameControlsWidget::set_loop);
    connect(ui.action_metadata, &QAction::triggered, parent, [this]{
        DocumentMetadataDialog(current_document.get(), this->parent).exec();
    });
    connect(ui.action_trace_bitmap, &QAction::triggered, parent, [this]{
        trace_dialog(current_shape());
    });
    connect(ui.action_object_to_path, &QAction::triggered, parent, [this]{to_path();});
    connect(ui.action_lottie_preview, &QAction::triggered, parent, [this]{preview_lottie("svg");});
    connect(ui.action_lottie_canvas_preview, &QAction::triggered, parent, [this]{preview_lottie("canvas");});
    connect(ui.action_svg_preview, &QAction::triggered, parent, [this]{preview_svg();});
    connect(ui.action_new_precomp, &QAction::triggered, parent, [this]{add_composition();});
    connect(ui.action_new_precomp_selection, &QAction::triggered, parent, [this]{
        objects_to_new_composition(comp, cleaned_selection(), &comp->shapes, -1);
    });
    connect(ui.menu_new_comp_layer, &QMenu::triggered, parent, [this](QAction* act){layer_new_comp(act);});
    connect(ui.action_align_hor_left_out,   &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin,  true);});
    connect(ui.action_align_hor_left,       &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Begin,  false);});
    connect(ui.action_align_hor_center,     &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::Center, false);});
    connect(ui.action_align_hor_right,      &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End,    false);});
    connect(ui.action_align_hor_right_out,  &QAction::triggered, parent, [this]{align(AlignDirection::Horizontal, AlignPosition::End,    true);});
    connect(ui.action_align_vert_top_out,   &QAction::triggered, parent, [this]{align(AlignDirection::Vertical,   AlignPosition::Begin,  true);});
    connect(ui.action_align_vert_top,       &QAction::triggered, parent, [this]{align(AlignDirection::Vertical,   AlignPosition::Begin,  false);});
    connect(ui.action_align_vert_center,    &QAction::triggered, parent, [this]{align(AlignDirection::Vertical,   AlignPosition::Center, false);});
    connect(ui.action_align_vert_bottom,    &QAction::triggered, parent, [this]{align(AlignDirection::Vertical,   AlignPosition::End,    false);});
    connect(ui.action_align_vert_bottom_out,&QAction::triggered, parent, [this]{align(AlignDirection::Vertical,   AlignPosition::End,    true);});
    connect(ui.action_import, &QAction::triggered, parent, [this]{import_file();});
    connect(ui.action_flip_view, &QAction::triggered, ui.graphics_view, &GlaxnimateGraphicsView::flip_horizontal);

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

    /// \todo Have some way of creating/connecting actions from the tools
    this->tool_actions["select"] = {
        ui.action_delete,
    };
    tool_widgets["edit"] = {
        ui.toolbar_node
    };
    this->tool_actions["edit"] = {
        ui.action_node_remove,
        ui.action_node_type_corner,
        ui.action_node_type_smooth,
        ui.action_node_type_symmetric,
        ui.action_segment_lines,
        ui.action_segment_curve,
        ui.action_node_add,
        ui.action_node_dissolve,
    };
    tools::EditTool* edit_tool = static_cast<tools::EditTool*>(tools::Registry::instance().tool("edit"));
    connect(ui.action_node_type_corner, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Corner);
    });
    connect(ui.action_node_type_smooth, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Smooth);
    });
    connect(ui.action_node_type_symmetric, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_set_vertex_type(math::bezier::Symmetrical);
    });
    connect(ui.action_node_remove, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_delete();
    });
    connect(ui.action_segment_lines, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_straighten();
    });
    connect(ui.action_segment_curve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_curve();
    });
    connect(ui.action_node_add, &QAction::triggered, parent, [edit_tool]{
        edit_tool->add_point_mode();
    });
    connect(ui.action_node_dissolve, &QAction::triggered, parent, [edit_tool]{
        edit_tool->selection_dissolve();
    });
    connect(edit_tool, &tools::EditTool::gradient_stop_changed, ui.fill_style_widget, &FillStyleWidget::set_gradient_stop);
    connect(edit_tool, &tools::EditTool::gradient_stop_changed, ui.stroke_style_widget, &StrokeStyleWidget::set_gradient_stop);


    // Item views
    comp_model.setSourceModel(&document_node_model);
    ui.view_document_node->setModel(&comp_model);
    ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    ui.view_document_node->setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &color_delegate);
    QObject::connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::currentChanged,
                        parent, &GlaxnimateWindow::document_treeview_current_changed);

    ui.view_document_node->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui.view_document_node, &QWidget::customContextMenuRequested, parent,
        [this](const QPoint& pos){
            auto index = ui.view_document_node->indexAt(pos);
            if ( auto node = document_node_model.node(comp_model.mapToSource(index)) )
                NodeMenu(node, this->parent, this->parent).exec(ui.view_document_node->mapToGlobal(pos));
        }
    );

    ui.view_properties->setModel(&property_model);
    ui.view_properties->setItemDelegateForColumn(1, &property_delegate);
    ui.view_properties->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.view_properties->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::selectionChanged, parent, &GlaxnimateWindow::document_treeview_selection_changed);

    ui.timeline_widget->set_controller(parent);

    // Tool buttons
    ui.btn_layer_add->setMenu(ui.menu_new_layer);

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
    connect(view_trans_widget, &ViewTransformWidget::flip_view, ui.action_flip_view, &QAction::trigger);

    // Graphics scene
    ui.graphics_view->setScene(&scene);
    ui.graphics_view->set_tool_target(parent);
    connect(&scene, &graphics::DocumentScene::node_user_selected, parent, &GlaxnimateWindow::scene_selection_changed);

    // Dialogs
    dialog_import_status = new IoStatusDialog(QIcon::fromTheme("document-open"), tr("Open File"), false, parent);
    dialog_export_status = new IoStatusDialog(QIcon::fromTheme("document-save"), tr("Save File"), false, parent);
    about_dialog = new AboutDialog(parent);

    // Recent files
    recent_files = app::settings::get<QStringList>("open_save", "recent_files");
    ui.action_open_last->setEnabled(!recent_files.isEmpty());
    reload_recent_menu();
    connect(ui.menu_open_recent, &QMenu::triggered, parent, &GlaxnimateWindow::document_open_recent);

    // Scripting
    connect(ui.console, &ScriptConsole::error, parent, [this](const QString& plugin, const QString& message){
        show_warning(plugin, message, app::log::Error);
    });
    ui.console->set_global("window", QVariant::fromValue(parent));
    init_plugins();

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
    ui.fill_style_widget->set_palette_model(&palette_model);
    ui.stroke_style_widget->set_palette_model(&palette_model);
    ui.document_swatch_widget->set_palette_model(&palette_model);

    connect(ui.document_swatch_widget, &DocumentSwatchWidget::needs_new_color, [this]{
        ui.document_swatch_widget->add_new_color(ui.fill_style_widget->current_color());
    });
    connect(ui.document_swatch_widget, &DocumentSwatchWidget::current_color_def, [this](model::BrushStyle* sty){
        set_color_def(sty, false);
    });
    connect(ui.document_swatch_widget, &DocumentSwatchWidget::secondary_color_def, [this](model::BrushStyle* sty){
        set_color_def(sty, true);
    });

    // Gradients
    ui.widget_gradients->set_window(parent);

    // Tab bar
    ui.tab_bar->setAutoHide(true);
    ui.tab_bar->setDocumentMode(true);
    ui.tab_bar->setDrawBase(false);
    ui.tab_bar->setExpanding(false);
    connect(ui.tab_bar, &QTabBar::currentChanged, parent, [this](int i){ switch_composition(i); });
    connect(ui.tab_bar, &QTabBar::tabCloseRequested, parent, [this](int i){ composition_close_request(i); });
    connect(ui.tab_bar, &ClickableTabBar::context_menu_requested, parent, [this](int i){ composition_context_menu(i); });

    // Align
    ui.separator_align_relative_to->setSeparator(true);
    ui.separator_align_horizontal->setSeparator(true);
    ui.separator_align_vertical->setSeparator(true);
    QActionGroup *align_relative = new QActionGroup(parent);
    align_relative->setExclusive(true);
    ui.action_align_to_canvas->setActionGroup(align_relative);
    ui.action_align_to_selection->setActionGroup(align_relative);
    ui.action_align_to_canvas_group->setActionGroup(align_relative);

    auto combo_align_to = new QComboBox(ui.dock_align->widget());
    combo_align_to->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui.dock_align_grid->addWidget(combo_align_to, 0, 0, 1, 3);

    action_combo(combo_align_to, ui.action_align_to_selection);
    action_combo(combo_align_to, ui.action_align_to_canvas);
    action_combo(combo_align_to, ui.action_align_to_canvas_group);
    connect(combo_align_to, qOverload<int>(&QComboBox::currentIndexChanged), parent, [combo_align_to](int i){
        combo_align_to->itemData(i).value<QAction*>()->setChecked(true);
    });

    int row = 1;
    ui.dock_align_grid->addWidget(action_button(ui.action_align_hor_left, ui.dock_align->widget()),         row, 0);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_hor_center, ui.dock_align->widget()),       row, 1);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_hor_right, ui.dock_align->widget()),        row, 2);
    row++;
    ui.dock_align_grid->addWidget(action_button(ui.action_align_hor_left_out, ui.dock_align->widget()),     row, 0);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_hor_right_out, ui.dock_align->widget()),    row, 2);
    row++;
    ui.dock_align_grid->addWidget(action_button(ui.action_align_vert_top, ui.dock_align->widget()),         row, 0);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_vert_center, ui.dock_align->widget()),      row, 1);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_vert_bottom, ui.dock_align->widget()),      row, 2);
    row++;
    ui.dock_align_grid->addWidget(action_button(ui.action_align_vert_top_out, ui.dock_align->widget()),     row, 0);
    ui.dock_align_grid->addWidget(action_button(ui.action_align_vert_bottom_out, ui.dock_align->widget()),  row, 2);
    row++;
    ui.dock_align_grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),        row, 0);

    // Arrange docks
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_layers);

    parent->tabifyDockWidget(ui.dock_timeline, ui.dock_properties);
    parent->tabifyDockWidget(ui.dock_properties, ui.dock_script_console);
    parent->tabifyDockWidget(ui.dock_script_console, ui.dock_logs);
    ui.dock_timeline->raise();

    parent->splitDockWidget(ui.dock_colors, ui.dock_swatches, Qt::Horizontal);
    parent->tabifyDockWidget(ui.dock_colors, ui.dock_stroke);
    parent->tabifyDockWidget(ui.dock_stroke, ui.dock_undo);
    parent->tabifyDockWidget(ui.dock_swatches, ui.dock_gradients);
    ui.dock_colors->raise();
    ui.dock_swatches->setVisible(false);
    ui.dock_gradients->setVisible(false);

    parent->tabifyDockWidget(ui.dock_tool_options, ui.dock_align);
    ui.dock_tool_options->raise();

    parent->resizeDocks(
        {ui.dock_layers},
        {1},
        Qt::Horizontal
    );
    parent->resizeDocks({ui.dock_tools}, {200}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_timeline}, {parent->height()/3}, Qt::Vertical);
    ui.dock_script_console->setVisible(false);
    ui.dock_logs->setVisible(false);

    // Load keyboard shortcuts
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_file);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_edit);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_document);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_tools);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_layers);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_path);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_new_layer);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_views);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_help);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_view);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_render_single_frame);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_playback);

#if 0
    // Auto Screenshots for docs
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
        ui.timeline_widget->load_state(app::settings::get<QByteArray>("ui", "timeline_splitter"));
    }

    // Hide tool widgets, as they might get shown by restoreState
    ui.toolbar_node->setVisible(false);
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
    ui.action_open_last->setEnabled(true);

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
    app::settings::set("ui", "timeline_splitter", ui.timeline_widget->save_state());
    app::settings::set("open_save", "recent_files", recent_files);

    ui.fill_style_widget->save_settings();
    ui.stroke_style_widget->save_settings();

    ui.console->save_settings();
    ui.console->clear_contexts();
}

void GlaxnimateWindow::Private::document_treeview_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    for ( const auto& index : deselected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = document_node_model.node(comp_model.mapToSource(index)) )
                scene.remove_selection(node);

    for ( const auto& index : selected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = document_node_model.node(comp_model.mapToSource(index)) )
                scene.add_selection(node);
}

void GlaxnimateWindow::Private::scene_selection_changed(const std::vector<model::DocumentNode*>& selected, const std::vector<model::DocumentNode*>& deselected)
{
    for ( model::DocumentNode* node : deselected )
    {
        ui.view_document_node->selectionModel()->select(
            comp_model.mapFromSource(document_node_model.node_index(node)),
            QItemSelectionModel::Deselect|QItemSelectionModel::Rows
        );
    }

    for ( model::DocumentNode* node : selected )
    {
        ui.view_document_node->selectionModel()->select(
            comp_model.mapFromSource(document_node_model.node_index(node)),
            QItemSelectionModel::Select|QItemSelectionModel::Rows
        );
    }

    if ( !selected.empty() )
    {
        ui.view_document_node->selectionModel()->setCurrentIndex(
            comp_model.mapFromSource(document_node_model.node_index(selected.back())),
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
    if ( !tool || tool == active_tool )
        return;

    if ( !tool->get_action()->isChecked() )
        tool->get_action()->setChecked(true);

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

    active_tool = tool;
    scene.set_active_tool(tool);
    ui.graphics_view->set_active_tool(tool);
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

void GlaxnimateWindow::Private::trace_dialog(model::ReferenceTarget* object)
{
    model::Image* bmp = 0;
    if ( object )
    {
        bmp = object->cast<model::Image>();
    }

    if ( !bmp )
    {
        for ( const auto& sel : scene.selection() )
        {
            if ( auto image = sel->cast<model::Image>() )
            {
                if ( bmp )
                {
                    show_warning(tr("Trace Bitmap"), tr("Only select one image"), app::log::Info);
                    return;
                }
                bmp = image;
            }
        }

        if ( !bmp )
        {
            show_warning(tr("Trace Bitmap"), tr("You need to select an image to trace"), app::log::Info);
            return;
        }
    }

    if ( !bmp->image.get() )
    {
        show_warning(tr("Trace Bitmap"), tr("You selected an image with no data"), app::log::Info);
        return;
    }

    TraceDialog dialog(bmp, parent);
    dialog.exec();
    if ( auto created = dialog.created() )
        ui.view_document_node->setCurrentIndex(
            comp_model.mapFromSource(document_node_model.node_index(created))
        );
}


void GlaxnimateWindow::Private::init_plugins()
{
    auto& par = plugin::PluginActionRegistry::instance();
    for ( auto act : par.enabled() )
    {
        ui.menu_plugins->addAction(par.make_qaction(act));
    }
    connect(&par, &plugin::PluginActionRegistry::action_added, parent, [this](plugin::ActionService* action, plugin::ActionService* before) {
        QAction* insert = nullptr;
        for ( auto act : ui.menu_plugins->actions() )
        {
            if ( act->data().value<plugin::ActionService*>() == before )
            {
                insert = act;
                break;
            }
        }

        ui.menu_plugins->insertAction(insert, plugin::PluginActionRegistry::instance().make_qaction(action));
    });
    connect(
        &plugin::PluginRegistry::instance(),
        &plugin::PluginRegistry::loaded,
        ui.console,
        &ScriptConsole::clear_contexts
    );

    plugin::PluginRegistry::instance().set_executor(ui.console);
}
