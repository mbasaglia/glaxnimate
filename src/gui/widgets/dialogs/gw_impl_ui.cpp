/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <QComboBox>
#include <QLabel>
#include <QScrollBar>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QActionGroup>
#include <QScreen>
#include <QDialogButtonBox>

#include "app/settings/keyboard_shortcuts.hpp"

#include "tools/base.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/repeater.hpp"
#include "model/shapes/trim.hpp"
#include "model/shapes/inflate_deflate.hpp"
#include "model/shapes/round_corners.hpp"
#include "model/shapes/offset_path.hpp"
#include "model/shapes/zig_zag.hpp"
#include "io/io_registry.hpp"

#include "widgets/dialogs/io_status_dialog.hpp"
#include "widgets/dialogs/about_dialog.hpp"
#include "widgets/dialogs/resize_dialog.hpp"
#include "widgets/dialogs/timing_dialog.hpp"
#include "widgets/dialogs/document_metadata_dialog.hpp"
#include "widgets/dialogs/trace_dialog.hpp"
#include "widgets/dialogs/startup_dialog.hpp"
#include "widgets/lottiefiles/lottiefiles_search_dialog.hpp"

#include "widgets/view_transform_widget.hpp"
#include "widgets/flow_layout.hpp"
#include "widgets/menus/node_menu.hpp"
#include "widgets/shape_style/shape_style_preview_widget.hpp"

#include "style/better_elide_delegate.hpp"
#include "tools/edit_tool.hpp"
#include "plugin/action.hpp"
#include "glaxnimate_app.hpp"
#include "settings/toolbar_settings.hpp"
#include "settings/document_templates.hpp"
#include "emoji/emoji_set_dialog.hpp"

using namespace glaxnimate::gui;


static QToolButton* action_button(QAction* action, QWidget* parent)
{
    auto button = new ScalableButton(parent);
    button->setDefaultAction(action);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    button->resize(16, 16);
    button->setMaximumSize(64, 64);
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

void GlaxnimateWindow::Private::setupUi(bool restore_state, bool debug, GlaxnimateWindow* parent)
{
    this->parent = parent;
    ui.setupUi(parent);
    redo_text = ui.action_redo->text();
    undo_text = ui.action_undo->text();

    init_actions();

    tools::Tool* to_activate = init_tools_ui();

    init_item_views();

    // Tool buttons
    ui.btn_layer_add->setMenu(ui.menu_new_layer);

    init_status_bar();

    // Graphics scene
    ui.canvas->setScene(&scene);
    ui.canvas->set_tool_target(parent);
    connect(&scene, &graphics::DocumentScene::node_user_selected, parent, &GlaxnimateWindow::scene_selection_changed);
    connect(ui.canvas, &Canvas::dropped, parent, [this](const QMimeData* d){dropped(d);});

    // Dialogs
    dialog_import_status = new IoStatusDialog(QIcon::fromTheme("document-open"), tr("Open File"), false, parent);
    dialog_export_status = new IoStatusDialog(QIcon::fromTheme("document-save"), tr("Save File"), false, parent);
    about_dialog = new AboutDialog(parent);

    // Recent files
    recent_files = app::settings::get<QStringList>("open_save", "recent_files");
    ui.action_open_last->setEnabled(!recent_files.isEmpty());
    reload_recent_menu();
    connect(ui.menu_open_recent, &QMenu::triggered, parent, &GlaxnimateWindow::document_open_recent);

    // Docks
    init_docks();

    // Menus
    init_menus();

    // Debug menu
    if ( debug )
        init_debug();

    // Initialize tools
    init_tools(to_activate);

    // Restore state
    // NOTE: keep at the end so we do this once all the widgets are in their default spots
    if ( restore_state )
        init_restore_state();


    // Toolbar settings
    parent->setToolButtonStyle(settings::ToolbarSettingsGroup::button_style);
    parent->setIconSize(settings::ToolbarSettingsGroup::icon_size());
    ui.toolbar_tools->setIconSize(settings::ToolbarSettingsGroup::tool_icon_size());
}

template<class T>
void GlaxnimateWindow::Private::add_modifier_menu_action(QMenu* menu)
{
    menu->addAction(T::static_tree_icon(), T::static_type_name_human(), [this]{
        auto layer = std::make_unique<T>(current_document.get());
        parent->layer_new_impl(std::move(layer));
    })->setObjectName("action_new_" + T::static_class_name().toLower());
}

void GlaxnimateWindow::Private::init_actions()
{
    // Standard Shortcuts
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
    ui.action_play->setShortcut(QKeySequence("Space", QKeySequence::PortableText));

    // Actions
    connect(ui.action_copy, &QAction::triggered, parent, &GlaxnimateWindow::copy);
    connect(ui.action_paste, &QAction::triggered, parent, &GlaxnimateWindow::paste);
    connect(ui.action_paste_as_composition, &QAction::triggered, parent, [this]{parent->paste_as_composition();});
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
    connect(ui.action_validate_discord, &QAction::triggered, parent, [this]{validate_discord();});
    connect(ui.action_resize, &QAction::triggered, parent, [this]{ ResizeDialog(this->parent).resize_composition(comp); });
    connect(ui.action_donate, &QAction::triggered, parent, &GlaxnimateWindow::help_donate);
    connect(ui.action_new_layer, &QAction::triggered, parent, [this]{layer_new_layer();});
    connect(ui.action_new_group, &QAction::triggered, parent, [this]{layer_new_group();});
    connect(ui.action_new_fill, &QAction::triggered, parent, [this]{layer_new_fill();});
    connect(ui.action_new_stroke, &QAction::triggered, parent, [this]{layer_new_stroke();});
    connect(ui.action_open_last, &QAction::triggered, parent, [this]{
        if ( !recent_files.isEmpty() )
        {
            // Avoid references to recent_files
            QString filename = recent_files[0];
            document_open_from_filename(filename);
        }
    });
    add_modifier_menu_action<model::Repeater>(ui.menu_new_layer);
    add_modifier_menu_action<model::Trim>(ui.menu_new_layer);
    add_modifier_menu_action<model::InflateDeflate>(ui.menu_new_layer);
    add_modifier_menu_action<model::RoundCorners>(ui.menu_new_layer);
    add_modifier_menu_action<model::OffsetPath>(ui.menu_new_layer);
    add_modifier_menu_action<model::ZigZag>(ui.menu_new_layer);
    connect(ui.action_import_image, &QAction::triggered, parent, [this]{import_image();});
    connect(ui.action_delete, &QAction::triggered, parent, &GlaxnimateWindow::delete_selected);
    connect(ui.action_export, &QAction::triggered, parent, &GlaxnimateWindow::document_export);
    connect(ui.action_export_as, &QAction::triggered, parent, &GlaxnimateWindow::document_export_as);
    connect(ui.action_export_sequence, &QAction::triggered, parent, &GlaxnimateWindow::document_export_sequence);
    connect(ui.action_document_cleanup, &QAction::triggered, parent, [this]{cleanup_document();});
    connect(ui.action_timing, &QAction::triggered, parent, [this]{
        TimingDialog(comp, this->parent).exec();
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

    connect(ui.play_controls,   &FrameControlsWidget::min_changed,    ui.play_controls_2, &FrameControlsWidget::set_min);
    connect(ui.play_controls,   &FrameControlsWidget::max_changed,    ui.play_controls_2, &FrameControlsWidget::set_max);
    connect(ui.play_controls,   &FrameControlsWidget::fps_changed,    ui.play_controls_2, &FrameControlsWidget::set_fps);
    connect(ui.play_controls,   &FrameControlsWidget::play_started,   ui.play_controls_2, &FrameControlsWidget::play);
    connect(ui.play_controls,   &FrameControlsWidget::play_stopped,   ui.play_controls_2, &FrameControlsWidget::pause);
    connect(ui.play_controls,   &FrameControlsWidget::loop_changed,   ui.play_controls_2, &FrameControlsWidget::set_loop);
    connect(ui.play_controls,   &FrameControlsWidget::frame_selected, ui.play_controls_2, &FrameControlsWidget::set_frame);
    connect(ui.play_controls_2, &FrameControlsWidget::play_started,   ui.play_controls,   &FrameControlsWidget::play);
    connect(ui.play_controls_2, &FrameControlsWidget::play_stopped,   ui.play_controls,   &FrameControlsWidget::pause);
    connect(ui.play_controls_2, &FrameControlsWidget::loop_changed,   ui.play_controls,   &FrameControlsWidget::set_loop);
    connect(ui.play_controls_2, &FrameControlsWidget::frame_selected, ui.play_controls,   &FrameControlsWidget::set_frame);
    connect(ui.play_controls, &FrameControlsWidget::min_changed, ui.scroll_time, &QSlider::setMinimum);
    connect(ui.play_controls, &FrameControlsWidget::max_changed, ui.scroll_time, &QSlider::setMaximum);
    connect(ui.play_controls, &FrameControlsWidget::frame_selected, ui.scroll_time, &QSlider::setValue);
    connect(ui.scroll_time, &QSlider::valueChanged, ui.play_controls_2, &FrameControlsWidget::frame_selected);

    connect(ui.action_metadata, &QAction::triggered, parent, [this]{
        DocumentMetadataDialog(current_document.get(), this->parent).exec();
    });
    connect(ui.action_trace_bitmap, &QAction::triggered, parent, [this]{
        trace_dialog(parent->current_shape());
    });
    connect(ui.action_object_to_path, &QAction::triggered, parent, [this]{to_path();});
    connect(ui.action_lottie_preview, &QAction::triggered, parent, [this]{preview_lottie("svg");});
    connect(ui.action_lottie_canvas_preview, &QAction::triggered, parent, [this]{preview_lottie("canvas");});
    connect(ui.action_svg_preview, &QAction::triggered, parent, [this]{preview_svg();});
    connect(ui.action_rive_preview, &QAction::triggered, parent, [this]{preview_rive();});
    connect(ui.action_new_precomp, &QAction::triggered, parent, [this]{add_composition();});
    connect(ui.action_new_precomp_selection, &QAction::triggered, parent, [this]{
        objects_to_new_composition(comp, cleaned_selection(), &comp->shapes, -1);
    });
    connect(ui.menu_new_comp_layer, &QMenu::triggered, parent, [this](QAction* act){layer_new_comp_action(act);});
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
    connect(ui.action_flip_view, &QAction::triggered, ui.canvas, &Canvas::flip_horizontal);
    connect(ui.action_text_put_on_path, &QAction::triggered, parent, [this]{text_put_on_path();});
    connect(ui.action_text_remove_from_path, &QAction::triggered, parent, [this]{text_remove_from_path();});
    connect(ui.action_insert_emoji, &QAction::triggered, parent, [this]{insert_emoji();});
    connect(ui.action_open_lottiefiles, &QAction::triggered, parent, [this]{import_from_lottiefiles();});

    // Undo Redo
    QObject::connect(ui.action_redo, &QAction::triggered, &parent->undo_group(), &QUndoGroup::redo);
    QObject::connect(&parent->undo_group(), &QUndoGroup::canRedoChanged, ui.action_redo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::redoTextChanged, ui.action_redo, [this](const QString& s){
        ui.action_redo->setText(redo_text.arg(s));
    });
    QObject::connect(ui.action_undo, &QAction::triggered, &parent->undo_group(), &QUndoGroup::undo);
    QObject::connect(&parent->undo_group(), &QUndoGroup::canUndoChanged, ui.action_undo, &QAction::setEnabled);
    QObject::connect(&parent->undo_group(), &QUndoGroup::undoTextChanged, ui.action_undo, [this](const QString& s){
        ui.action_undo->setText(undo_text.arg(s));
    });
    ui.view_undo->setGroup(&parent->undo_group());


#ifdef Q_OS_WIN32
    // Can't get emoji_data.cpp to compile on windows for qt6 for some reason
    // the compiler errors out without message
    ui.action_insert_emoji->setEnabled(false);
#endif
}

tools::Tool* GlaxnimateWindow::Private::init_tools_ui()
{
    // Tool Actions
    QActionGroup *tool_actions = new QActionGroup(parent);
    tool_actions->setExclusive(true);

    dock_tools_layout = new FlowLayout();
    ui.dock_tools_layout_parent->insertLayout(0, dock_tools_layout);
    tools::Tool* to_activate = nullptr;
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

            ui.toolbar_tools->addAction(action);

            button->resize(16, 16);
            dock_tools_layout->addWidget(button);

            QWidget* widget = tool.second->get_settings_widget();
            widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            ui.tool_settings_widget->addWidget(widget);

            if ( !to_activate )
            {
                to_activate = tool.second.get();
                action->setChecked(true);
            }
        }
        ui.menu_tools->addSeparator();
        ui.toolbar_tools->addSeparator();
    }

    ui.toolbar_node->setVisible(false);
    ui.toolbar_node->setEnabled(false);

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

    return to_activate;
}

void GlaxnimateWindow::Private::init_item_views()
{
    // Item views
    ui.view_document_node->set_base_model(&document_node_model);
    QObject::connect(ui.view_document_node, &LayerView::current_node_changed,
                        parent, &GlaxnimateWindow::document_treeview_current_changed);

    ui.view_document_node->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui.view_document_node, &QWidget::customContextMenuRequested, parent,
        [this](const QPoint& pos){
            auto index = ui.view_document_node->indexAt(pos);
            if ( auto node = ui.view_document_node->node(index) )
                NodeMenu(node, this->parent, this->parent).exec(ui.view_document_node->mapToGlobal(pos));
        }
    );

    ui.view_properties->setModel(&property_model);
    ui.view_properties->setItemDelegateForColumn(item_models::PropertyModelSingle::ColumnValue, &property_delegate);
    ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnName, QHeaderView::ResizeToContents);
    ui.view_properties->header()->setSectionResizeMode(item_models::PropertyModelSingle::ColumnValue, QHeaderView::Stretch);

    connect(ui.view_document_node, &LayerView::selection_changed, parent, &GlaxnimateWindow::document_treeview_selection_changed);

    ui.timeline_widget->set_controller(parent);

    asset_model.setSourceModel(&document_node_model);
    ui.view_assets->setModel(&asset_model);
    ui.view_assets->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName-1, QHeaderView::Stretch);
    ui.view_assets->header()->hideSection(item_models::DocumentNodeModel::ColumnVisible-1);
    ui.view_assets->header()->hideSection(item_models::DocumentNodeModel::ColumnLocked-1);
    ui.view_assets->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnUsers-1, QHeaderView::ResizeToContents);
    connect(ui.view_assets, &CustomTreeView::customContextMenuRequested, parent, [this](const QPoint& pos){
        auto node = document_node_model.node(asset_model.mapToSource(ui.view_assets->indexAt(pos)));
        if ( node )
            NodeMenu(node, this->parent, this->parent).exec(ui.view_assets->mapToGlobal(pos));
    });


    connect(ui.timeline_widget, &CompoundTimelineWidget::current_node_changed, parent, [this](model::VisualNode* node){
        timeline_current_node_changed(node);
    });
    connect(ui.timeline_widget, &CompoundTimelineWidget::selection_changed, parent, [this](const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected){
        selection_changed(selected, deselected);
    });
}

static QWidget* status_bar_separator()
{
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

void GlaxnimateWindow::Private::init_status_bar()
{
    // Recording
    widget_recording = new QWidget();
    ui.status_bar->addPermanentWidget(widget_recording);
    QHBoxLayout* lay = new QHBoxLayout;
    widget_recording->setLayout(lay);
    lay->setContentsMargins(0, 0, 0, 0);
    widget_recording->setVisible(false);

    QLabel* label_recording_icon = new QLabel();
    label_recording_icon->setPixmap(QIcon::fromTheme("media-record").pixmap(ui.status_bar->height()));
    lay->addWidget(label_recording_icon);

    label_recording = new QLabel();
    label_recording->setText(tr("Recording Keyframes"));
    lay->addWidget(label_recording);

    lay->addWidget(status_bar_separator());

    // X: ... Y: ...
    label_mouse_pos = new QLabel();
    ui.status_bar->addPermanentWidget(label_mouse_pos);
    QFont font;
    font.setFamily("monospace");
    font.setStyleHint(QFont::Monospace);
    label_mouse_pos->setFont(font);
    mouse_moved(QPointF(0, 0));
    connect(ui.canvas, &Canvas::mouse_moved, parent, [this](const QPointF& p){mouse_moved(p);});

    ui.status_bar->addPermanentWidget(status_bar_separator());

    // Current Style
    widget_current_style = new ShapeStylePreviewWidget();
    ui.status_bar->addPermanentWidget(widget_current_style);
    widget_current_style->setFixedSize(ui.status_bar->height(), ui.status_bar->height());
    connect(ui.fill_style_widget, &FillStyleWidget::current_color_changed,
            widget_current_style, &ShapeStylePreviewWidget::set_fill_color);
    connect(ui.stroke_style_widget, &StrokeStyleWidget::color_changed,
            widget_current_style, &ShapeStylePreviewWidget::set_stroke_color);
    ui.status_bar->addPermanentWidget(status_bar_separator());
    widget_current_style->set_fill_color(ui.fill_style_widget->current_color());
    widget_current_style->set_stroke_color(ui.stroke_style_widget->current_color());

    // Transform widget
    view_trans_widget = new ViewTransformWidget(ui.status_bar);
    ui.status_bar->addPermanentWidget(view_trans_widget);
    connect(view_trans_widget, &ViewTransformWidget::zoom_changed, ui.canvas, &Canvas::set_zoom);
    connect(ui.canvas, &Canvas::zoomed, view_trans_widget, &ViewTransformWidget::set_zoom);
    connect(view_trans_widget, &ViewTransformWidget::zoom_in, ui.canvas, &Canvas::zoom_in);
    connect(view_trans_widget, &ViewTransformWidget::zoom_out, ui.canvas, &Canvas::zoom_out);
    connect(view_trans_widget, &ViewTransformWidget::angle_changed, ui.canvas, &Canvas::set_rotation);
    connect(ui.canvas, &Canvas::rotated, view_trans_widget, &ViewTransformWidget::set_angle);
    connect(view_trans_widget, &ViewTransformWidget::view_fit, parent, &GlaxnimateWindow::view_fit);
    connect(view_trans_widget, &ViewTransformWidget::flip_view, ui.action_flip_view, &QAction::trigger);
}

void GlaxnimateWindow::Private::init_docks()
{
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
        connect(ui.widget_gradients, &GradientListWidget::selected, parent, [this](model::BrushStyle* sty, bool secondary){
            set_brush_reference(sty, secondary);
    });

    // Fill/Stroke
    connect(ui.fill_style_widget, &FillStyleWidget::current_color_changed, parent, [this]{style_change_event();});
    connect(ui.fill_style_widget, &FillStyleWidget::secondary_color_changed, parent, [this]{style_change_event();});
    connect(ui.stroke_style_widget, &StrokeStyleWidget::pen_style_changed, parent, [this]{style_change_event();});

    // Tab bar
    connect(ui.tab_bar, &CompositionTabBar::switch_composition, parent, &GlaxnimateWindow::switch_composition);
    connect(ui.timeline_widget, &CompoundTimelineWidget::switch_composition, parent, &GlaxnimateWindow::switch_composition);

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
    ui.dock_align->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Layout
    QActionGroup *layout_actions = new QActionGroup(parent);
    layout_actions->setExclusive(true);
    for ( const auto& action : ui.menu_layout->actions() )
    {
        action->setActionGroup(layout_actions);
        connect(action, &QAction::triggered, parent, [this]{layout_update();});
    }

    auto preset = LayoutPreset(app::settings::get<int>("ui", "layout"));

    switch ( preset )
    {
        case LayoutPreset::Unknown:
        case LayoutPreset::Auto:
            layout_auto();
            break;
        case LayoutPreset::Wide:
            layout_wide();
            break;
        case LayoutPreset::Compact:
            layout_compact();
            break;
        case LayoutPreset::Medium:
            layout_medium();
            break;
        case LayoutPreset::Custom:
            layout_auto();
            app::settings::set("ui", "layout", int(LayoutPreset::Custom));
            ui.action_layout_custom->setChecked(true);
            break;
    }
}

void GlaxnimateWindow::Private::layout_update()
{
    if ( ui.action_layout_wide->isChecked() )
        layout_wide();
    else if ( ui.action_layout_compact->isChecked() )
        layout_compact();
    else if ( ui.action_layout_custom->isChecked() )
        layout_custom();
    else if ( ui.action_layout_medium->isChecked() )
        layout_medium();
    else
        layout_auto();
}

void GlaxnimateWindow::Private::layout_medium()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_timeline);
    parent->tabifyDockWidget(ui.dock_timeline, ui.dock_properties);
    parent->tabifyDockWidget(ui.dock_properties, ui.dock_script_console);
    parent->tabifyDockWidget(ui.dock_script_console, ui.dock_logs);
    parent->tabifyDockWidget(ui.dock_logs, ui.dock_time_slider);
    ui.dock_timeline->raise();
    ui.dock_time_slider->setVisible(false);
    ui.dock_timeline->setVisible(true);
    ui.dock_properties->setVisible(true);

    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_snippets);

    // Bottom Right
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_layers);
    parent->tabifyDockWidget(ui.dock_layers, ui.dock_gradients);
    parent->tabifyDockWidget(ui.dock_gradients, ui.dock_swatches);
    parent->tabifyDockWidget(ui.dock_swatches, ui.dock_assets);
    ui.dock_gradients->raise();
    ui.dock_gradients->setVisible(true);
    ui.dock_assets->setVisible(false);
    ui.dock_swatches->setVisible(false);
    ui.dock_layers->setVisible(true);


    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, ui.dock_colors);
    parent->tabifyDockWidget(ui.dock_colors, ui.dock_stroke);
    parent->tabifyDockWidget(ui.dock_stroke, ui.dock_undo);
    ui.dock_colors->raise();
    ui.dock_colors->setVisible(true);
    ui.dock_stroke->setVisible(true);
    ui.dock_undo->setVisible(true);


    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tools);

    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tool_options);
    parent->tabifyDockWidget(ui.dock_tool_options, ui.dock_align);
    ui.dock_tool_options->raise();
    ui.dock_tool_options->setVisible(true);
    ui.dock_align->setVisible(true);

    // Resize
    parent->resizeDocks({ui.dock_snippets}, {1}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_layers}, {1}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_tools}, {200}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_tool_options, ui.dock_align, ui.dock_tools}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({ui.dock_timeline}, {300}, Qt::Vertical);
    ui.dock_script_console->setVisible(false);
    ui.dock_logs->setVisible(false);
    ui.dock_tools->setVisible(false);
    ui.dock_snippets->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1440, 900);

    app::settings::set("ui", "layout", int(LayoutPreset::Medium));
    ui.action_layout_medium->setChecked(true);
}

void GlaxnimateWindow::Private::layout_wide()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_timeline);
    parent->tabifyDockWidget(ui.dock_timeline, ui.dock_properties);
    parent->tabifyDockWidget(ui.dock_properties, ui.dock_script_console);
    parent->tabifyDockWidget(ui.dock_script_console, ui.dock_logs);
    parent->tabifyDockWidget(ui.dock_logs, ui.dock_time_slider);
    ui.dock_timeline->raise();
    ui.dock_time_slider->setVisible(false);
    ui.dock_timeline->setVisible(true);
    ui.dock_properties->setVisible(true);

    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_snippets);

    // Bottom Right
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_layers);
    parent->tabifyDockWidget(ui.dock_layers, ui.dock_gradients);
    parent->tabifyDockWidget(ui.dock_gradients, ui.dock_swatches);
    parent->tabifyDockWidget(ui.dock_swatches, ui.dock_assets);
    ui.dock_gradients->raise();
    ui.dock_gradients->setVisible(true);
    ui.dock_assets->setVisible(false);
    ui.dock_swatches->setVisible(false);
    ui.dock_layers->setVisible(true);


    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, ui.dock_colors);
    parent->tabifyDockWidget(ui.dock_colors, ui.dock_stroke);
    parent->tabifyDockWidget(ui.dock_stroke, ui.dock_undo);
    ui.dock_colors->raise();
    ui.dock_colors->setVisible(true);
    ui.dock_stroke->setVisible(true);
    ui.dock_undo->setVisible(true);


    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tools);

    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tool_options);
    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_align);
    ui.dock_tool_options->setVisible(true);
    ui.dock_align->setVisible(true);

    // Resize
    parent->resizeDocks({ui.dock_snippets}, {1}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_layers}, {1}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_tools}, {200}, Qt::Horizontal);
    parent->resizeDocks({ui.dock_tool_options, ui.dock_align, ui.dock_tools}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({ui.dock_timeline}, {1080/3}, Qt::Vertical);
    ui.dock_script_console->setVisible(false);
    ui.dock_logs->setVisible(false);
    ui.dock_tools->setVisible(false);
    ui.dock_snippets->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1920, 1080);

    app::settings::set("ui", "layout", int(LayoutPreset::Wide));
    ui.action_layout_wide->setChecked(true);
}

void GlaxnimateWindow::Private::layout_compact()
{
    // Bottom
    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_time_slider);
    parent->tabifyDockWidget(ui.dock_time_slider, ui.dock_timeline);
    parent->tabifyDockWidget(ui.dock_timeline, ui.dock_script_console);
    parent->tabifyDockWidget(ui.dock_script_console, ui.dock_logs);
    ui.dock_time_slider->raise();
    ui.dock_time_slider->setVisible(true);
    ui.dock_timeline->setVisible(false);
    ui.dock_logs->setVisible(false);

    parent->addDockWidget(Qt::BottomDockWidgetArea, ui.dock_snippets);

    // Right
    parent->addDockWidget(Qt::RightDockWidgetArea, ui.dock_colors);
    parent->tabifyDockWidget(ui.dock_colors, ui.dock_stroke);
    parent->tabifyDockWidget(ui.dock_stroke, ui.dock_layers);
    parent->tabifyDockWidget(ui.dock_layers, ui.dock_properties);
    parent->tabifyDockWidget(ui.dock_properties, ui.dock_undo);
    parent->tabifyDockWidget(ui.dock_undo, ui.dock_gradients);
    parent->tabifyDockWidget(ui.dock_gradients, ui.dock_swatches);
    parent->tabifyDockWidget(ui.dock_swatches, ui.dock_assets);
    ui.dock_colors->raise();
    ui.dock_colors->setVisible(true);
    ui.dock_stroke->setVisible(true);
    ui.dock_gradients->setVisible(false);
    ui.dock_assets->setVisible(false);
    ui.dock_swatches->setVisible(false);
    ui.dock_undo->setVisible(false);
    ui.dock_properties->setVisible(true);
    ui.dock_layers->setVisible(true);

    // Left
    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tools);

    parent->addDockWidget(Qt::LeftDockWidgetArea, ui.dock_tool_options);
    parent->tabifyDockWidget(ui.dock_tool_options, ui.dock_align);
    ui.dock_tool_options->raise();
    ui.dock_tool_options->setVisible(true);
    ui.dock_align->setVisible(true);

    // Resize
    parent->resizeDocks({ui.dock_tool_options, ui.dock_align, ui.dock_tools}, {1, 1, 4000}, Qt::Vertical);
    parent->resizeDocks({ui.dock_time_slider}, {64}, Qt::Vertical);
    ui.dock_script_console->setVisible(false);
    ui.dock_logs->setVisible(false);
    ui.dock_tools->setVisible(false);
    ui.dock_snippets->setVisible(false);

    // Resize parent to have a reasonable default size
    parent->resize(1366, 768);

    app::settings::set("ui", "layout", int(LayoutPreset::Compact));
    ui.action_layout_compact->setChecked(true);
}

void GlaxnimateWindow::Private::layout_auto()
{
    auto real_estate = qApp->primaryScreen()->availableSize();
    if ( real_estate.width() >= 1920 && real_estate.height() >= 1080 )
        layout_wide();
    else if ( real_estate.width() >= 1440 && real_estate.height() >= 900 )
        layout_medium();
    else
        layout_compact();

    app::settings::set("ui", "layout", int(LayoutPreset::Auto));
    ui.action_layout_automatic->setChecked(true);
}

void GlaxnimateWindow::Private::layout_custom()
{
    init_restore_state();
    app::settings::set("ui", "layout", int(LayoutPreset::Custom));
    ui.action_layout_custom->setChecked(true);
}

void GlaxnimateWindow::Private::init_template_menu()
{
    ui.menu_new_from_template->clear();

    for ( const auto& templ : settings::DocumentTemplates::instance().templates() )
        ui.menu_new_from_template->addAction(settings::DocumentTemplates::instance().create_action(templ, ui.menu_new_from_template));
}

void GlaxnimateWindow::Private::init_menus()
{
    // Menu Views
    for ( QDockWidget* wid : parent->findChildren<QDockWidget*>() )
    {
        QAction* act = wid->toggleViewAction();
        act->setIcon(wid->windowIcon());
        ui.menu_views->addAction(act);
        wid->setStyle(&dock_style);
    }

    // Menu Toolbars
    for ( QToolBar* wid : parent->findChildren<QToolBar*>() )
    {
        QAction* act = wid->toggleViewAction();
        ui.menu_toolbars->addAction(act);
        wid->setStyle(&dock_style);
    }

    // Load keyboard shortcuts
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_file);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_edit);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_tools);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_view);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_views);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_document);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_render_single_frame);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_playback);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_layers);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_new_layer);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_object);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_path);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_text);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_plugins);
    GlaxnimateApp::instance()->shortcuts()->add_menu(ui.menu_help);

    // Menu Templates
    init_template_menu();

    connect(&settings::DocumentTemplates::instance(), &settings::DocumentTemplates::loaded, parent, [this]{
        init_template_menu();
    });

    connect(&settings::DocumentTemplates::instance(), &settings::DocumentTemplates::create_from, parent,
        [this](const settings::DocumentTemplate& templ){
            if ( !close_document() )
                return;

            bool ok = false;
            setup_document_ptr(templ.create(&ok));
            if ( !ok )
                show_warning(tr("New from Template"), tr("Could not load template"));
        }
    );

    connect(ui.action_save_as_template, &QAction::triggered, parent, [this]{
        bool ok = true;

        QString old_name = comp->name.get();
        QString name = QInputDialog::getText(parent, tr("Save as Template"), tr("Name"), QLineEdit::Normal, old_name, &ok);
        if ( !ok )
            return;

        comp->name.set(name);
        if ( !settings::DocumentTemplates::instance().save_as_template(current_document.get()) )
            show_warning(tr("Save as Template"), tr("Could not save template"));
        comp->name.set(old_name);
    });
}


void GlaxnimateWindow::Private::init_tools(tools::Tool* to_activate)
{
    tools::Event event{ui.canvas, &scene, parent};
    for ( const auto& grp : tools::Registry::instance() )
    {
        for ( const auto& tool : grp.second )
        {
            tool.second->retranslate();
            tool.second->initialize(event);

            if ( to_activate == tool.second.get() )
                switch_tool(tool.second.get());
        }
    }
}

void GlaxnimateWindow::Private::init_restore_state()
{
    parent->restoreState(app::settings::get<QByteArray>("ui", "window_state"));
    ui.timeline_widget->load_state(app::settings::get<QByteArray>("ui", "timeline_splitter"));
    parent->restoreGeometry(app::settings::get<QByteArray>("ui", "window_geometry"));

    // Hide tool widgets, as they might get shown by restoreState
    ui.toolbar_node->setVisible(false);
    ui.toolbar_node->setEnabled(false);
}

void GlaxnimateWindow::Private::retranslateUi(QMainWindow* parent)
{
    ui.retranslateUi(parent);
    label_recording->setText(tr("Recording Keyframes"));

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
    ui.canvas->view_fit();
}

void GlaxnimateWindow::Private::reload_recent_menu()
{
    QIcon icon(GlaxnimateApp::instance()->data_file("images/glaxnimate-file.svg"));
    ui.menu_open_recent->clear();
    for ( const auto& recent : recent_files )
    {
        QAction* act = new QAction(icon, QFileInfo(recent).fileName(), ui.menu_open_recent);
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
    ui.canvas->set_active_tool(tool);

    if ( auto old_wid = ui.tool_settings_widget->currentWidget() )
        old_wid->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    auto new_wid = tool->get_settings_widget();
    new_wid->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ui.tool_settings_widget->setCurrentWidget(new_wid);
    if ( active_tool->group() == tools::Registry::Draw || active_tool->group() == tools::Registry::Shape )
        widget_current_style->clear_gradients();
}

void GlaxnimateWindow::Private::switch_tool_action(QAction* action)
{
    switch_tool(action->data().value<tools::Tool*>());
}

void GlaxnimateWindow::Private::status_message(const QString& message, int duration)
{
    ui.status_bar->showMessage(message, duration);
}

void GlaxnimateWindow::Private::trace_dialog(model::DocumentNode* object)
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
        ui.view_document_node->set_current_node(created);
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
        QAction* qaction = plugin::PluginActionRegistry::instance().make_qaction(action);
        ui.menu_plugins->insertAction(insert, qaction);

        app::settings::ShortcutGroup* group = GlaxnimateApp::instance()->shortcuts()->find_group(ui.menu_plugins->menuAction()->iconText());
        if (group)
            group->actions.push_back(GlaxnimateApp::instance()->shortcuts()->add_action(qaction));
    });

    connect(&par, &plugin::PluginActionRegistry::action_removed, parent, [this](plugin::ActionService* action) {
        QString slug = "action_plugin_" + action->plugin()->data().name.toLower() + "_" + action->label.toLower();
        app::settings::ShortcutAction* act = GlaxnimateApp::instance()->shortcuts()->action(slug);
        GlaxnimateApp::instance()->shortcuts()->remove_action(act);
    });

    connect(
        &plugin::PluginRegistry::instance(),
        &plugin::PluginRegistry::loaded,
        ui.console,
        &ScriptConsole::clear_contexts
    );

    plugin::PluginRegistry::instance().set_executor(ui.console);
}

void GlaxnimateWindow::Private::mouse_moved(const QPointF& pos)
{
    label_mouse_pos->setText(tr("X: %1 Y: %2").arg(pos.x(), 8, 'f', 3).arg(pos.y(), 8, 'f', 3));
}

void GlaxnimateWindow::Private::show_startup_dialog()
{
    if ( !app::settings::get<bool>("ui", "startup_dialog") )
        return;

    StartupDialog dialog(parent);
    connect(&dialog, &StartupDialog::open_recent, parent, &GlaxnimateWindow::document_open);
    connect(&dialog, &StartupDialog::open_browse, parent, &GlaxnimateWindow::document_open_dialog);
    if ( dialog.exec() )
        setup_document_ptr(dialog.create());
}


void GlaxnimateWindow::Private::drop_file(const QString& file)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(tr("Drop File"));
    QIcon icon = QIcon::fromTheme("dialog-question");
    dialog.setWindowIcon(icon);
    QVBoxLayout lay;
    dialog.setLayout(&lay);

    QHBoxLayout lay1;
    QLabel label_icon;
    label_icon.setPixmap(icon.pixmap(64));
    lay1.addWidget(&label_icon);
    QLabel label_text;
    label_text.setText(tr("Add to current file?"));
    label_text.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lay1.addWidget(&label_text);
    lay.addLayout(&lay1);

    QHBoxLayout lay2;
    QPushButton btn1(tr("Add as Object"));
    lay2.addWidget(&btn1);
    connect(&btn1, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        drop_document(file, false);
        dialog.accept();
    });
    QPushButton btn2(tr("Add as Composition"));
    lay2.addWidget(&btn2);
    connect(&btn2, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        drop_document(file, true);
        dialog.accept();
    });
    QPushButton btn3(tr("Open"));
    lay2.addWidget(&btn3);
    connect(&btn3, &QPushButton::clicked, &dialog, [&dialog, this, &file]{
        document_open_from_filename(file);
        dialog.accept();
    });
    lay.addLayout(&lay2);

    dialog.exec();
}

void GlaxnimateWindow::Private::insert_emoji()
{
    emoji::EmojiSetDialog dialog;
    if ( !dialog.exec() || dialog.selected_svg().isEmpty() )
        return;
    import_file(dialog.selected_svg(), {{"forced_size", comp->size()}});
}

void GlaxnimateWindow::Private::import_from_lottiefiles()
{
    LottieFilesSearchDialog dialog;
    if ( !dialog.exec() )
        return;
    io::Options options;
    options.format = io::IoRegistry::instance().from_slug("lottie");
    options.filename = dialog.selected_name() + ".json";
    load_remote_document(dialog.selected_url(), options, dialog.result() == LottieFilesSearchDialog::Open);
}

QVariant GlaxnimateWindow::Private::choose_option(const QString& label, const QVariantMap& options, const QString& title) const
{
    QDialog dialog(parent);
    if ( !title.isEmpty() )
        dialog.setWindowTitle(title);

    QVBoxLayout* layout = new QVBoxLayout();
    dialog.setLayout(layout);

    QLabel qlabel;
    qlabel.setText(label);
    layout->addWidget(&qlabel);

    QComboBox box;
    layout->addWidget(&box);
    int count = 0;
    for ( auto it = options.begin(); it != options.end(); ++it )
    {
        box.insertItem(count++, it.key(), *it);
    }

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QDialogButtonBox buttons;
    buttons.addButton(QDialogButtonBox::Ok);
    buttons.addButton(QDialogButtonBox::Cancel);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(&buttons);

    if ( !dialog.exec() )
        return {};

    return box.currentData();
}
