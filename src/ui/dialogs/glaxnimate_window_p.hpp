#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include <QGraphicsView>
#include <QToolButton>
#include <QMessageBox>

#include "QtColorWidgets/color_palette_model.hpp"
#include "QtColorWidgets/color_delegate.hpp"

#include "ui_glaxnimate_window.h"
#include "glaxnimate_window.hpp"

#include "app_info.hpp"
#include "app/settings/settings.hpp"
#include "app/application.hpp"

#include "command/layer_commands.hpp"

#include "model/document.hpp"
#include "model/item_models/document_node_model.hpp"
#include "model/item_models/property_model.hpp"
#include "model/graphics/document_scene.hpp"

#include "ui/dialogs/import_export_dialog.hpp"
#include "ui/dialogs/io_status_dialog.hpp"
#include "ui/dialogs/about_dialog.hpp"
#include "ui/style/dock_widget_style.hpp"
#include "ui/style/property_delegate.hpp"
#include "ui/widgets/glaxnimate_graphics_view.hpp"
#include "ui/widgets/view_transform_widget.hpp"
#include "ui/widgets/scalable_button.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"

#include "scripting/script_engine.hpp"
#include "scripting/plugin.hpp"

namespace {


void color_hsv_h(QColor& c, int v) { c.setHsv(v, c.hsvSaturation(), c.value(), c.alpha()); }
void color_hsv_s(QColor& c, int v) { c.setHsv(c.hsvHue(), v, c.value(), c.alpha()); }
void color_hsv_v(QColor& c, int v) { c.setHsv(c.hsvHue(), c.hsvSaturation(), v, c.alpha()); }

void color_hsl_h(QColor& c, int v) { c.setHsv(v, c.hslSaturation(), c.value(), c.alpha()); }
void color_hsl_s(QColor& c, int v) { c.setHsl(c.hslHue(), v, c.lightness(), c.alpha()); }
void color_hsl_l(QColor& c, int v) { c.setHsl(c.hslHue(), c.hslSaturation(), v, c.alpha()); }

void color_r(QColor& c, int v) { c.setRed(v); }
void color_g(QColor& c, int v) { c.setGreen(v); }
void color_b(QColor& c, int v) { c.setBlue(v); }
void color_a(QColor& c, int v) { c.setAlpha(v); }

void color_c(QColor& c, int v) { c.setCmyk(v, c.magenta(), c.yellow(), c.black(), c.alpha()); }
void color_m(QColor& c, int v) { c.setCmyk(c.cyan(), v, c.yellow(), c.black(), c.alpha()); }
void color_y(QColor& c, int v) { c.setCmyk(c.cyan(), c.magenta(), v, c.black(), c.alpha()); }
void color_k(QColor& c, int v) { c.setCmyk(c.cyan(), c.magenta(), c.yellow(), v, c.alpha()); }

} // namespace


class GlaxnimateWindow::Private
{
public:
    Ui::GlaxnimateWindow ui;

    std::unique_ptr<model::Document> current_document;

    model::DocumentNodeModel document_node_model;
    model::PropertyModel property_model;
    model::graphics::DocumentScene scene;

    GlaxnimateWindow* parent = nullptr;

    bool updating_color = false;

    QStringList recent_files;

    std::vector<scripting::ScriptContext> script_contexts;

    // "set and forget" kida variables
    int tool_rows = 3; ///< @todo setting
    QString undo_text;
    QString redo_text;
    color_widgets::ColorPaletteModel palette_model;
    color_widgets::ColorDelegate color_delegate;
    PropertyDelegate property_delegate;
    DockWidgetStyle dock_style;
    ViewTransformWidget* view_trans_widget;
    bool started = false;
    IoStatusDialog* dialog_import_status;
    IoStatusDialog* dialog_export_status;
    AboutDialog* about_dialog;

    void setup_document(const QString& filename)
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
        property_model.set_object(current_document->animation());

        scene.set_document(current_document.get());

        // Scripting
        script_contexts.clear();

        // Title
        QObject::connect(current_document.get(), &model::Document::filename_changed, parent, &GlaxnimateWindow::refresh_title);
        QObject::connect(&current_document->undo_stack(), &QUndoStack::cleanChanged, parent, &GlaxnimateWindow::refresh_title);
        refresh_title();
    }


    void setup_document_new(const QString& filename)
    {
        setup_document(filename);

        current_document->animation()->name.set(current_document->animation()->type_name_human());
        auto layer = current_document->animation()->make_layer<model::ShapeLayer>();
        current_document->animation()->width.set(app::settings::get<int>("defaults", "width"));
        current_document->animation()->height.set(app::settings::get<int>("defaults", "height"));
        current_document->animation()->fps.set(app::settings::get<int>("defaults", "fps"));
        float duration = app::settings::get<float>("defaults", "duration");
        int out_point = current_document->animation()->fps.get() * duration;
        current_document->animation()->out_point.set(out_point);
        layer->out_point.set(out_point);
        layer->name.set(layer->type_name_human());
        model::Layer* ptr = layer.get();
        current_document->animation()->add_layer(std::move(layer), 0);

        QDir path = app::settings::get<QString>("open_save", "path");

        auto opts = current_document->io_options();
        opts.path = path;
        current_document->set_io_options(opts);

        ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
        view_fit();
    }

    bool setup_document_open(const io::Options& options)
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
        if ( !current_document->animation()->layers.empty() )
            ui.view_document_node->setCurrentIndex(document_node_model.node_index(&current_document->animation()->layers[0]));

        return ok;
    }

    void refresh_title()
    {
        QString title = current_document->filename();
        if ( !current_document->undo_stack().isClean() )
            title += " *";
        parent->setWindowTitle(title);
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
                save_document(false, false);
            else if ( result == QMessageBox::Cancel )
                return false;

            // Prevent signals on the destructor
            current_document->undo_stack().clear();
        }

        document_node_model.clear_document();
        property_model.clear_document();
        scene.clear_document();

        return true;
    }


    bool save_document(bool force_dialog, bool overwrite_doc)
    {
        io::Options opts = current_document->io_options();

        if ( !opts.format || !opts.format->can_save() )
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


    void setupUi(GlaxnimateWindow* parent)
    {
        this->parent = parent;
        ui.setupUi(parent);
        redo_text = ui.action_redo->text();
        undo_text = ui.action_undo->text();

        // Standard Shorcuts
        ui.action_new->setShortcut(QKeySequence::New);
        ui.action_open->setShortcut(QKeySequence::Open);
        ui.action_close->setShortcut(QKeySequence::Close);
        ui.action_reload->setShortcut(QKeySequence::Refresh);
        ui.action_save->setShortcut(QKeySequence::Save);
        ui.action_save_as->setShortcut(QKeySequence::SaveAs);
        ui.action_quit->setShortcut(QKeySequence::Quit);
        ui.action_copy->setShortcut(QKeySequence::Copy);
        ui.action_cut->setShortcut(QKeySequence::Cut);
        ui.action_paste->setShortcut(QKeySequence::Paste);
        ui.action_select_all->setShortcut(QKeySequence::SelectAll);
        ui.action_undo->setShortcut(QKeySequence::Undo);
        ui.action_redo->setShortcut(QKeySequence::Redo);

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

        int row = 0;
        int column = 0;
        for ( QAction* action : ui.menu_tools->actions() )
        {
            if ( action->isSeparator() )
                continue;

            action->setActionGroup(tool_actions);

            ScalableButton *button = new ScalableButton(ui.dock_tools_contents);

            button->setIcon(action->icon());
            button->setCheckable(true);
            button->setChecked(action->isChecked());

            update_tool_button(action, button);
            QObject::connect(action, &QAction::changed, button, [action, button, this](){
                update_tool_button(action, button);
            });
            QObject::connect(button, &QToolButton::toggled, action, &QAction::setChecked);
            QObject::connect(action, &QAction::toggled, button, &QToolButton::setChecked);

            button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            ui.dock_tools_layout->addWidget(button, row, column);

            column++;
            if ( column >= tool_rows )
            {
                column = 0;
                row++;
            }
        }

        // Colors
        update_color(Qt::black, true, nullptr);
        ui.palette_widget->setModel(&palette_model);
        palette_model.setSearchPaths(app::Application::instance()->data_paths_unchecked("palettes"));

        // Item views
        ui.view_document_node->setModel(&document_node_model);
        ui.view_document_node->header()->setSectionResizeMode(model::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
        ui.view_document_node->header()->setSectionResizeMode(model::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
        ui.view_document_node->setItemDelegateForColumn(model::DocumentNodeModel::ColumnColor, &color_delegate);
        QObject::connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::currentChanged,
                         parent, &GlaxnimateWindow::document_treeview_current_changed);

        ui.view_properties->setModel(&property_model);
        ui.view_properties->setItemDelegateForColumn(1, &property_delegate);

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

        // Graphics scene
        ui.graphics_view->setScene(&scene);

        // dialogs
        dialog_import_status = new IoStatusDialog(QIcon::fromTheme("document-open"), tr("Open File"), false, parent);
        dialog_export_status = new IoStatusDialog(QIcon::fromTheme("document-save"), tr("Save File"), false, parent);
        about_dialog = new AboutDialog(parent);

        // Recent files
        recent_files = app::settings::get<QStringList>("open_save", "recent_files");
        reload_recent_menu();
        connect(ui.menu_open_recent, &QMenu::triggered, parent, &GlaxnimateWindow::document_open_recent);

        // Scripting
        parent->tabifyDockWidget(ui.dock_script_console, ui.dock_timeline);
        ui.dock_script_console->setVisible(false);

        ui.console_input->setHistory(app::settings::get<QStringList>("scripting", "history"));

        for ( const auto& engine : scripting::ScriptEngineFactory::instance().engines() )
        {
            ui.console_language->addItem(engine->label());
            if ( engine->slug() == "python" )
                ui.console_language->setCurrentIndex(ui.console_language->count()-1);
        }

        // Plugins
        auto& par = scripting::PluginActionRegistry::instance();
        for ( auto act : par.enabled() )
        {
            ui.menu_plugins->addAction(par.make_qaction(act));
        }
        connect(&par, &scripting::PluginActionRegistry::action_added, parent, [this](scripting::ActionService* action) {
            ui.menu_plugins->addAction(scripting::PluginActionRegistry::instance().make_qaction(action));
        });

        // Restore state
        // NOTE: keep at the end so we do this once all the widgets are in their default spots
        parent->restoreGeometry(app::settings::get<QByteArray>("ui", "window_geometry"));
        parent->restoreState(app::settings::get<QByteArray>("ui", "window_state"));

    }

    void retranslateUi(QMainWindow* parent)
    {
        ui.retranslateUi(parent);
        redo_text = ui.action_redo->text();
        undo_text = ui.action_undo->text();

        ui.action_undo->setText(redo_text.arg(current_document->undo_stack().undoText()));
        ui.action_redo->setText(redo_text.arg(current_document->undo_stack().redoText()));
    }

    void update_tool_button(QAction* action, QToolButton* button)
    {
        button->setText(action->text());
        button->setToolTip(action->text());
    }

    void update_color_slider(color_widgets::GradientSlider* slider, const QColor& c,
                             void (*func)(QColor&, int), int val, int min = 0, int max = 255)
    {
        QColor c1 = c;
        (*func)(c1, min);
        QColor c2 = c;
        (*func)(c2, (min+max)/2);
        QColor c3 = c;
        (*func)(c3, max);
        slider->setColors({c1, c2, c3});
        slider->setValue(val);
    }

    void update_color_hue_slider(color_widgets::HueSlider* slider, const QColor& c, int hue)
    {
        slider->setColorSaturation(c.saturationF());
        slider->setColorValue(c.valueF());
        slider->setValue(hue);
    }

    QColor current_color()
    {
        return ui.color_preview->color();
    }

    void update_color(const QColor& c, bool alpha, QObject* source)
    {
        if ( updating_color )
            return;
        updating_color = true;

        QColor col = c;
        if ( !alpha )
            col.setAlpha(current_color().alpha());

        int hue = col.hsvHue();
        if ( hue == -1 )
            hue = col.hslHue();
        if ( hue == -1 )
            hue = current_color().hue();
        if ( hue == -1 )
            hue = 0;

        // main
        ui.color_preview->setColor(col);
        ui.color_line_edit->setColor(col);
        update_color_slider(ui.slider_alpha, col, color_a, col.alpha());

        // HSV
        if ( source != ui.color_hsv )
            ui.color_hsv->setColor(col);
        update_color_hue_slider(ui.slider_hsv_hue, col, hue);
        update_color_slider(ui.slider_hsv_sat, col, color_hsv_s, col.hsvSaturation());
        update_color_slider(ui.slider_hsv_value, col, color_hsv_v, col.value());

        // HSL
        if ( source != ui.color_hsl )
            ui.color_hsl->setColor(col);
        update_color_hue_slider(ui.slider_hsl_hue, col, hue);
        update_color_slider(ui.slider_hsl_sat, col, color_hsl_s, col.hslSaturation());
        update_color_slider(ui.slider_hsl_light, col, color_hsl_l, col.lightness());

        // RGB
        update_color_slider(ui.slider_rgb_r, col, color_r, col.red());
        update_color_slider(ui.slider_rgb_g, col, color_g, col.green());
        update_color_slider(ui.slider_rgb_b, col, color_b, col.blue());

        // CMYK
        update_color_slider(ui.slider_cmyk_c, col, color_c, col.cyan());
        update_color_slider(ui.slider_cmyk_m, col, color_m, col.magenta());
        update_color_slider(ui.slider_cmyk_y, col, color_y, col.yellow());
        update_color_slider(ui.slider_cmyk_k, col, color_k, col.black());

        // Palette
        if ( source != ui.palette_widget )
            ui.palette_widget->setCurrentColor(col);
        ui.palette_widget->setDefaultColor(col);

        updating_color = false;
    }

    void update_color_component(int val, QObject* sender)
    {
        if ( updating_color )
            return;

        void (*func)(QColor&, int) = nullptr;

        if ( sender->objectName() == "slider_alpha" )
            func = color_a;
        else if ( sender->objectName() == "slider_hsv_hue" )
            func = color_hsv_h;
        else if ( sender->objectName() == "slider_hsv_sat" )
            func = color_hsv_s;
        else if ( sender->objectName() == "slider_hsv_value" )
            func = color_hsv_v;
        else if ( sender->objectName() == "slider_hsl_hue" )
            func = color_hsl_h;
        else if ( sender->objectName() == "slider_hsl_sat" )
            func = color_hsl_s;
        else if ( sender->objectName() == "slider_hsl_light" )
            func = color_hsl_l;
        else if ( sender->objectName() == "slider_rgb_r" )
            func = color_r;
        else if ( sender->objectName() == "slider_rgb_g" )
            func = color_g;
        else if ( sender->objectName() == "slider_rgb_b" )
            func = color_b;
        else if ( sender->objectName() == "slider_cmyk_c" )
            func = color_c;
        else if ( sender->objectName() == "slider_cmyk_m" )
            func = color_m;
        else if ( sender->objectName() == "slider_cmyk_y" )
            func = color_y;
        else if ( sender->objectName() == "slider_cmyk_k" )
            func = color_k;

        if ( !func )
            return;

        QColor c = current_color();
        (*func)(c, val);
        update_color(c, true, sender);
    }

    model::Composition* current_composition()
    {
        model::DocumentNode* curr = current_document_node();
        if ( curr )
        {
            if ( auto curr_comp = qobject_cast<model::Composition*>(curr) )
                return curr_comp;

            if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
                return curr_lay->composition();
        }
        return current_document->animation();
    }

    model::Layer* current_layer()
    {
        model::DocumentNode* curr = current_document_node();
        if ( curr )
        {
            if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
                return curr_lay;
        }
        return nullptr;
    }

    model::DocumentNode* current_document_node()
    {
        return document_node_model.node(ui.view_document_node->currentIndex());
    }

    template<class LayerT>
    void layer_new()
    {
        layer_new_impl(current_composition()->make_layer<LayerT>());
    }

    void layer_new_impl(std::unique_ptr<model::Layer> layer)
    {
        model::Composition* composition = current_composition();

        QString base_name = layer->type_name_human();
        QString name = base_name;

        int n = 0;
        for ( int i = 0; i < composition->layers.size(); i++ )
        {
            if ( composition->layers[i].name.get() == name )
            {
                n += 1;
                name = tr("%1 %2").arg(base_name).arg(n);
                i = -1;
            }
        }

        layer->name.set(name);

        layer->out_point.set(current_document->animation()->out_point.get());

        model::Layer* ptr = layer.get();

        int position = composition->layer_position(current_layer());
        current_document->undo_stack().push(new command::AddLayer(composition, std::move(layer), position));

        ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
    }

    void layer_delete()
    {
        /// @todo Remove shapes / precompositions
        model::DocumentNode* curr = current_document_node();
        if ( !curr )
            return;

        if ( auto curr_lay = qobject_cast<model::Layer*>(curr) )
        {
            current_document->undo_stack().push(new command::RemoveLayer(curr_lay->composition(), curr_lay));
        }
    }

    void document_treeview_current_changed(const QModelIndex& index)
    {
        if ( auto node = document_node_model.node(index) )
            property_model.set_object(node);
    }

    void view_fit()
    {
        ui.graphics_view->view_fit(
            QRect(
                -32,
                -32,
                current_document->animation()->width.get() + 64,
                current_document->animation()->height.get() + 64
            )
        );
    }

    void document_open()
    {
        io::Options options = current_document->io_options();

        ImportExportDialog dialog(options, ui.centralwidget->parentWidget());
        if ( dialog.import_dialog() )
            setup_document_open(dialog.io_options());

    }

    void shutdown()
    {
        app::settings::set("ui", "window_geometry", parent->saveGeometry());
        app::settings::set("ui", "window_state", parent->saveState());
        app::settings::set("open_save", "recent_files", recent_files);

        QStringList history = ui.console_input->history();
        int max_history = app::settings::get<int>("scripting", "max_history");
        if ( history.size() > max_history )
            history.erase(history.begin() + max_history, history.end());
        app::settings::set("scripting", "history", history);
        script_contexts.clear();
    }

    void reload_recent_menu()
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

    void most_recent_file(const QString& s)
    {
        recent_files.removeAll(s);
        recent_files.push_front(s);

        int max = app::settings::get<int>("open_save", "max_recent_files");
        if ( recent_files.size() > max )
            recent_files.erase(recent_files.begin() + max, recent_files.end());

        reload_recent_menu();
    }

    void document_open_from_filename(const QString& filename)
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

    void show_warning(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Warning)
    {
        QMessageBox warning(parent);
        warning.setWindowTitle(title);
        warning.setText(message);
        warning.setStandardButtons(QMessageBox::Ok);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.setIcon(icon);
        warning.exec();
    }

    void help_about()
    {
        about_dialog->show();
    }

    void console_commit(QString text)
    {
        if ( text.isEmpty() )
            return;

        text = text.replace("\n", " ");

        if ( script_contexts.empty() )
        {
            create_script_context();
            if ( script_contexts.empty() )
                return;
        }

        auto c = ui.console_output->textCursor();
        c.clearSelection();
        ui.console_output->setTextCursor(c);

        ui.console_output->append(text);
        auto ctx = script_contexts[ui.console_language->currentIndex()].get();
        try {
            QString out = ctx->eval_to_string(text);
            if ( !out.isEmpty() )
                ui.console_output->append(out);
        } catch ( const scripting::ScriptError& err ) {
            QColor col = ui.console_output->textColor();
            qDebug() << col << col.isValid();
            ui.console_output->setTextColor(Qt::red);
            ui.console_output->append(err.message());
            ui.console_output->setTextColor(col);
        }
        ui.console_input->setText("");
    }


    void create_script_context()
    {
        for ( const auto& engine : scripting::ScriptEngineFactory::instance().engines() )
        {
            auto ctx = engine->create_context();
            ctx->expose("window", parent);
            ctx->expose("document", current_document.get());
            script_contexts.push_back(std::move(ctx));
        }
    }

};

#endif // GLAXNIMATEWINDOW_P_H
