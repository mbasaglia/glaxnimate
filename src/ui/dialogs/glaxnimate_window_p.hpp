#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include <QGraphicsView>
#include <QToolButton>

#include "QtColorWidgets/color_palette_model.hpp"
#include "QtColorWidgets/color_delegate.hpp"

#include "ui_glaxnimate_window.h"
#include "glaxnimate_window.hpp"
#include "app/app_info.hpp"
#include "model/document.hpp"
#include "model/item_models/document_node_model.hpp"
#include "model/item_models/property_model.hpp"
#include "model/graphics/document_scene.hpp"
#include "ui/dialogs/import_export_dialog.hpp"
#include "ui/style/dock_widget_style.hpp"
#include "ui/style/property_delegate.hpp"
#include "ui/widgets/glaxnimate_graphics_view.hpp"
#include "command/layer_commands.hpp"

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

    int tool_rows = 3;
    std::vector<std::unique_ptr<model::Document>> documents;
    QString undo_text;
    QString redo_text;
    bool updating_color = false;
    color_widgets::ColorPaletteModel palette_model;
    model::DocumentNodeModel document_node_model;
    color_widgets::ColorDelegate color_delegate;
    model::PropertyModel property_model;
    PropertyDelegate property_delegate;
    DockWidgetStyle dock_style;
    GlaxnimateWindow* parent = nullptr;


    model::Document* current_document()
    {
        int index = ui.tab_widget->currentIndex();
        if ( index == -1 )
            return nullptr;

        return documents[index].get();
    }

    model::Document* create_document(const QString& filename)
    {
        documents.push_back(std::make_unique<model::Document>(filename));
        auto widget = new GlaxnimateGraphicsView();
        ui.tab_widget->addTab(widget, QIcon::fromTheme("video-x-generic"), filename);
        model::Document* doc = documents.back().get();
        auto refresh_slot = [this, doc](){parent->refresh_title(doc);};
        QObject::connect(doc, &model::Document::filename_changed, parent, refresh_slot);
        QObject::connect(&doc->undo_stack(), &QUndoStack::cleanChanged, parent, refresh_slot);
        widget->setScene(&doc->graphics_scene());

        switch_to_document(doc);

        auto layer = std::make_unique<model::ShapeLayer>(&doc->animation());
        model::Layer* ptr = layer.get();
        doc->animation().add_layer(std::move(layer), 0);
        ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));

        return doc;
    }

    int document_index(model::Document* doc)
    {
        for ( int i = 0; i < int(documents.size()); i++ )
            if ( documents[i].get() == doc )
                return i;
        return -1;

    }

    void refresh_title(model::Document* doc)
    {
        QString title = doc->filename();
        if ( !doc->undo_stack().isClean() )
            title += " *";
        ui.tab_widget->setTabText(document_index(doc), title);
        if ( doc == current_document() )
            parent->setWindowTitle(title);
    }


    bool save_document(model::Document* doc, bool force_dialog, bool overwrite_doc)
    {
        if ( !doc )
            return false;

        io::SavedIoOptions opts = doc->export_options();

        if ( !opts.method )
            force_dialog = true;

        if ( force_dialog )
        {
            ImportExportDialog dialog(ui.centralwidget->parentWidget());

            if ( !dialog.export_dialog(doc) )
                return false;

            if ( !dialog.options_dialog() )
                return false;

            opts = dialog.io_options();
        }

        // TODO progess/error dialogs
        QFile file(opts.filename);
        file.open(QFile::WriteOnly);
        if ( !opts.method->process(file, opts.filename, doc, opts.options) )
            return false;

        doc->undo_stack().setClean();

        if ( overwrite_doc )
            doc->set_export_options(opts);

        return true;
    }

    void switch_to_document(model::Document* document)
    {
        // TODO disconnect old document
        // Undo Redo
        QObject::connect(ui.action_redo, &QAction::triggered, &document->undo_stack(), &QUndoStack::redo);
        QObject::connect(&document->undo_stack(), &QUndoStack::canRedoChanged, ui.action_redo, &QAction::setEnabled);
        QObject::connect(&document->undo_stack(), &QUndoStack::redoTextChanged, ui.action_redo, [this](const QString& s){
            ui.action_redo->setText(redo_text.arg(s));
        });
        ui.action_redo->setEnabled(document->undo_stack().canRedo());
        ui.action_redo->setText(redo_text.arg(document->undo_stack().redoText()));

        QObject::connect(ui.action_undo, &QAction::triggered, &document->undo_stack(), &QUndoStack::undo);
        QObject::connect(&document->undo_stack(), &QUndoStack::canUndoChanged, ui.action_undo, &QAction::setEnabled);
        QObject::connect(&document->undo_stack(), &QUndoStack::undoTextChanged, ui.action_undo, [this](const QString& s){
            ui.action_undo->setText(undo_text.arg(s));
        });
        ui.action_undo->setEnabled(document->undo_stack().canUndo());
        ui.action_undo->setText(redo_text.arg(document->undo_stack().undoText()));

        // Tree view
        // TODO Store collapsed state
        document_node_model.set_document(document);

        property_model.set_document(document);
        // TODO keep selection
        property_model.set_object(&document->animation());

        // Title
        refresh_title(document);
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

            QToolButton *button = new QToolButton(ui.dock_tools_contents);

            button->setIcon(action->icon());
            button->setCheckable(true);
            button->setChecked(action->isChecked());

            update_tool_button(action, button);
            QObject::connect(action, &QAction::changed, button, [action, button, this](){
                update_tool_button(action, button);
            });
            QObject::connect(button, &QToolButton::toggled, action, &QAction::setChecked);
            QObject::connect(action, &QAction::toggled, button, &QToolButton::setChecked);

            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            button->installEventFilter(parent);

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
        palette_model.setSearchPaths(AppInfo::instance().data_paths_unchecked("palettes"));

        // Item views
        ui.view_document_node->setModel(&document_node_model);
        ui.view_document_node->header()->setSectionResizeMode(model::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
        ui.view_document_node->header()->setSectionResizeMode(model::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
        ui.view_document_node->setItemDelegateForColumn(model::DocumentNodeModel::ColumnColor, &color_delegate);
        QObject::connect(ui.view_document_node->selectionModel(), &QItemSelectionModel::currentChanged,
                         parent, &GlaxnimateWindow::document_treeview_current_changed);

        ui.view_properties->setModel(&property_model);
        ui.view_properties->setItemDelegateForColumn(1, &property_delegate);
    }

    void retranslateUi(QMainWindow* parent)
    {
        ui.retranslateUi(parent);
        redo_text = ui.action_redo->text();
        undo_text = ui.action_undo->text();
    }

    void update_tool_button(QAction* action, QToolButton* button)
    {
        button->setText(action->text());
        button->setToolTip(action->text());
    }

    bool eventFilter(QObject* object, QEvent* event)
    {
        QToolButton *btn = qobject_cast<QToolButton*>(object);
        if ( btn && event->type() == QEvent::Resize )
        {
            int target = std::max(16, std::min(128, btn->size().width() - 10));
            btn->setIconSize(QSize(target, target));
//             QSize best(0, 0);
//             for ( const auto& sz : btn->icon().availableSizes() )
//             {
//                 if ( sz.width() > best.width() && sz.width() <= target )
//                     best = sz;
//             }
//             if ( best.width() > 0 )
//                 btn->setIconSize(best);
        }

        return false;
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
                return curr_lay->composition;
        }
        return &current_document()->animation();
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
        if ( !current_document() )
            return;

        layer_new_impl(std::make_unique<LayerT>(current_composition()));
    }

    void layer_new_impl(std::unique_ptr<model::Layer> layer)
    {
        model::Composition* composition = current_composition();

        QString base_name = layer->docnode_name();
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

        model::Layer* ptr = layer.get();

        int position = composition->layer_position(current_layer());
        current_document()->undo_stack().push(new command::AddLayer(composition, std::move(layer), position));

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
            current_document()->undo_stack().push(new command::RemoveLayer(curr_lay->composition, curr_lay));
        }
    }

    void document_treeview_current_changed(const QModelIndex& index)
    {
        if ( auto node = document_node_model.node(index) )
            property_model.set_object(node);
    }
};

#endif // GLAXNIMATEWINDOW_P_H
