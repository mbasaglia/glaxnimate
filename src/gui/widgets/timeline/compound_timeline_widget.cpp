/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "compound_timeline_widget.hpp"
#include "ui_compound_timeline_widget.h"

#include <QMenu>
#include <QScrollBar>
#include <QClipboard>
#include <QMimeData>
#include <QSignalBlocker>
#include <QActionGroup>

#include "math/bezier/meta.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "item_models/property_model_full.hpp"
#include "item_models/comp_filter_model.hpp"

#include "glaxnimate_app.hpp"

#include "style/property_delegate.hpp"
#include "style/fixed_height_delegate.hpp"
#include "widgets/dialogs/keyframe_editor_dialog.hpp"

#ifndef Q_OS_ANDROID
    #include "widgets/menus/node_menu.hpp"
#endif
#include "widgets/menus/animated_property_menu.hpp"
#include "widgets/timeline/keyframe_transition_data.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class CompoundTimelineWidget::Private
{
public:
    void setupUi(CompoundTimelineWidget* parent)
    {
        ui.setupUi(parent);
        comp_model.setSourceModel(&property_model);

        QPalette prop_pal = ui.properties->palette();
        prop_pal.setBrush(
            QPalette::Inactive,
            QPalette::Highlight,
            prop_pal.brush(QPalette::Active, QPalette::Highlight)
        );
        prop_pal.setBrush(
            QPalette::Inactive,
            QPalette::HighlightedText,
            prop_pal.brush(QPalette::Active, QPalette::HighlightedText)
        );
        ui.properties->setPalette(prop_pal);

        ui.properties->setModel(&comp_model);
        ui.timeline->set_model(&comp_model, &property_model, ui.properties);
        connect(&property_model, &QAbstractItemModel::dataChanged,
                ui.properties->viewport(), (void (QWidget::*)())&QWidget::update);

        ui.properties->setItemDelegate(&fixed_height_delegate);
        ui.properties->setItemDelegateForColumn(item_models::PropertyModelFull::ColumnValue, &property_delegate);
        ui.properties->setItemDelegateForColumn(item_models::PropertyModelFull::ColumnColor, &color_delegate);

        ui.properties->header()->setSectionResizeMode(item_models::PropertyModelFull::ColumnName, QHeaderView::ResizeToContents);
        ui.properties->header()->setSectionResizeMode(item_models::PropertyModelFull::ColumnValue, QHeaderView::Stretch);
        ui.properties->header()->setSectionResizeMode(item_models::PropertyModelFull::ColumnColor, QHeaderView::ResizeToContents);
        ui.properties->header()->setSectionResizeMode(item_models::PropertyModelFull::ColumnLocked, QHeaderView::ResizeToContents);
        ui.properties->header()->setSectionResizeMode(item_models::PropertyModelFull::ColumnVisible, QHeaderView::ResizeToContents);
        ui.properties->header()->moveSection(item_models::PropertyModelFull::ColumnColor, 0);
        ui.properties->header()->moveSection(item_models::PropertyModelFull::ColumnVisible, 1);
        ui.properties->header()->moveSection(item_models::PropertyModelFull::ColumnLocked, 2);

        ui.properties->setUniformRowHeights(true);
        ui.properties->header()->setFixedHeight(ui.timeline->header_height());
        property_delegate.set_forced_height(ui.timeline->header_height());
        fixed_height_delegate.set_height(ui.timeline->row_height());
        ui.properties->setRootIsDecorated(true);

        ui.properties->verticalScrollBar()->installEventFilter(parent);
        ui.properties->verticalScrollBar()->setSingleStep(ui.scrollbar->singleStep());
        ui.properties->verticalScrollBar()->setPageStep(ui.scrollbar->pageStep());
        connect(ui.properties->verticalScrollBar(), &QScrollBar::valueChanged, ui.scrollbar, &QScrollBar::setValue);
        connect(ui.scrollbar, &QScrollBar::valueChanged, ui.properties->verticalScrollBar(), &QScrollBar::setValue);
        connect(ui.properties->verticalScrollBar(), &QScrollBar::rangeChanged, ui.scrollbar, &QScrollBar::setRange);
        connect(ui.scrollbar, &QScrollBar::valueChanged, parent, &CompoundTimelineWidget::on_scroll);

        connect(ui.timeline, &TimelineWidget::line_clicked, parent, &CompoundTimelineWidget::select_line);
        connect(ui.properties->selectionModel(), &QItemSelectionModel::currentChanged, parent, &CompoundTimelineWidget::select_index);
        connect(ui.properties->selectionModel(), &QItemSelectionModel::selectionChanged, parent, &CompoundTimelineWidget::_on_selection_changed);

        ui.action_add_keyframe->setIcon(QIcon::fromTheme("keyframe-add"));
        connect(ui.action_add_keyframe, &QAction::triggered, parent, &CompoundTimelineWidget::add_keyframe);

        connect(parent, &QWidget::customContextMenuRequested, parent, &CompoundTimelineWidget::custom_context_menu);

        setup_menu(parent);

    }

    void setup_menu(CompoundTimelineWidget* parent)
    {
        for ( int i = 0; i < KeyframeTransitionData::count; i++ )
        {
            auto data = KeyframeTransitionData::from_index(i, KeyframeTransitionData::Full);
            actions_enter[i].setIcon(data.icon());
            actions_enter[i].setActionGroup(&enter);
            actions_enter[i].setData(data.variant());

            actions_leave[i].setIcon(data.icon());
            actions_leave[i].setActionGroup(&exit);
            actions_leave[i].setData(data.variant());
        }

        menu_keyframe.addAction(&action_kf_remove);
        action_kf_remove.setIcon(QIcon::fromTheme("keyframe-remove"));
        connect(&action_kf_remove, &QAction::triggered, parent, &CompoundTimelineWidget::remove_keyframe);


        menu_keyframe.addAction(&action_kf_copy);
        action_kf_copy.setIcon(QIcon::fromTheme("keyframe-duplicate"));
        connect(&action_kf_copy, &QAction::triggered, parent, &CompoundTimelineWidget::copy_keyframe);

        action_kf_paste.setIcon(QIcon::fromTheme("edit-paste"));
        menu_keyframe.addAction(&action_kf_paste);

        menu_keyframe.addSeparator();

        menu_keyframe.addAction(&action_kf_remove_all);
        action_kf_remove_all.setIcon(QIcon::fromTheme("edit-clear-all"));
        connect(&action_kf_remove_all, &QAction::triggered, &menu_property, &AnimatedPropertyMenu::remove_all_keyframes);

        action_enter = menu_keyframe.addSeparator();
        for ( QAction* ac : enter.actions() )
        {
            ac->setCheckable(true);
            menu_keyframe.addAction(ac);
            connect(ac, &QAction::triggered, parent, &CompoundTimelineWidget::keyframe_action_enter);
        }

        action_exit = menu_keyframe.addSeparator();
        for ( QAction* ac : exit.actions() )
        {
            ac->setCheckable(true);
            menu_keyframe.addAction(ac);
            connect(ac, &QAction::triggered, parent, &CompoundTimelineWidget::keyframe_action_exit);
        }

        retranslate_menu();
    }

    void retranslate_menu()
    {
        action_enter->setText(tr("Transition From Previous"));
        action_exit->setText(tr("Transition To Next"));


        for ( int i = 0; i < KeyframeTransitionData::count; i++ )
        {
            if ( i == model::KeyframeTransition::Custom )
            {
                actions_enter[i].setText(tr("Custom..."));
                actions_leave[i].setText(actions_enter[i].text());
            }
            else
            {
                actions_leave[i].setText(KeyframeTransitionData::from_index(i, KeyframeTransitionData::Start).name);
                actions_enter[i].setText(KeyframeTransitionData::from_index(i, KeyframeTransitionData::Finish).name);
            }
        }

        action_kf_remove.setText(tr("Remove Keyframe"));
        action_kf_remove_all.setText(tr("Clear Animations"));

        action_kf_copy.setText(tr("Copy Keyframe"));
        action_kf_paste.setText(tr("Paste Keyframe"));
    }

    void retranslateUi(CompoundTimelineWidget* parent)
    {
        ui.retranslateUi(parent);
        retranslate_menu();
    }

    void clear_menu_data()
    {
        menu_property.set_property(nullptr);
        menu_kf_enter = nullptr;
        menu_kf_exit = nullptr;
    }

    void keyframe_action(CompoundTimelineWidget* parent, model::KeyframeBase* keyframe, bool before_transition, QAction* action)
    {

        if ( !keyframe || !menu_property.property() )
            return;

        int index = menu_property.property()->keyframe_index(keyframe);
        if ( index == -1 )
            return;

        QVariant data = action->data();
        if ( data.isValid() && data.toInt() != model::KeyframeTransition::Custom )
        {
            menu_property.property()->object()->push_command(
                new command::SetKeyframeTransition(
                    menu_property.property(), index, data.value<model::KeyframeTransition::Descriptive>(),
                    before_transition ? keyframe->transition().before() : keyframe->transition().after(),
                    before_transition
                )
            );
        }
        else
        {
            KeyframeEditorDialog keyframe_editor(keyframe->transition(), parent);
            keyframe_editor.setWindowModality(Qt::ApplicationModal);
            if ( keyframe_editor.exec() )
            {
                menu_property.property()->object()->push_command(
                    new command::SetKeyframeTransition(
                        menu_property.property(), index,
                        keyframe_editor.transition()
                    )
                );
            }
        }
    }

    void toggle_paste()
    {
        action_kf_paste.setEnabled(menu_property.can_paste());
    }

    void emit_click(CompoundTimelineWidget* parent, QModelIndex index)
    {
        model::VisualNode* node = nullptr;
        do
        {
            node = property_model.visual_node(comp_model.mapToSource(index));
            index = index.parent();
        }
        while ( !node && index.isValid() );

        if ( node )
            emit parent->current_node_changed(node);
    }

    model::VisualNode* index_node_or_parent(QModelIndex property_index)
    {
        while ( property_index.isValid() )
        {
            auto item = property_model.item(property_index);
            model::Object* obj = item.object;
            if ( !obj && item.property )
            {
                obj = item.property->object();
                if ( !obj )
                    return nullptr;
            }

            if ( auto visual = obj->cast<model::VisualNode>() )
                return visual;

            if ( obj->is_instance<model::Asset>() )
                return nullptr;

            property_index = property_index.parent();
        }

        return nullptr;
    }


    Ui_CompoundTimelineWidget ui;
    item_models::PropertyModelFull property_model;
    style::PropertyDelegate property_delegate;
    color_widgets::ColorDelegate color_delegate;
    style::FixedHeightDelegate fixed_height_delegate;
    item_models::CompFilterModel comp_model;

    AnimatedPropertyMenu menu_property;

    QAction* action_enter;
    QAction* action_exit;
    QAction action_kf_copy;
    QAction action_kf_paste;
    QAction action_kf_remove;
    QAction action_kf_remove_all;

    std::array<QAction, KeyframeTransitionData::count> actions_enter;
    std::array<QAction, KeyframeTransitionData::count> actions_leave;

    QMenu menu_keyframe;
    QActionGroup enter{&menu_keyframe};
    QActionGroup exit{&menu_keyframe};

    model::KeyframeBase* menu_kf_enter = nullptr;
    model::KeyframeBase* menu_kf_exit = nullptr;

    GlaxnimateWindow* window = nullptr;
};

CompoundTimelineWidget::CompoundTimelineWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->setupUi(this);
    connect(d->ui.tab_bar, &CompositionTabBar::switch_composition, this, &CompoundTimelineWidget::switch_composition);
    connect(&d->property_model, &QAbstractItemModel::rowsRemoved, this, &CompoundTimelineWidget::rows_removed);
}


CompoundTimelineWidget::~CompoundTimelineWidget() = default;

void CompoundTimelineWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void CompoundTimelineWidget::set_composition(model::Composition* comp)
{
    QSignalBlocker g(d->ui.tab_bar);
    d->ui.tab_bar->set_current_composition(comp);
    d->comp_model.set_composition(comp);

    on_scroll(d->ui.scrollbar->value());
}

void CompoundTimelineWidget::set_current_node(model::DocumentNode* node)
{
    QModelIndex index = d->comp_model.mapFromSource(d->property_model.object_index(node));
    d->ui.properties->expand(index);
    d->ui.properties->setCurrentIndex(index);
    d->clear_menu_data();
}

void CompoundTimelineWidget::set_document(model::Document* document)
{
    d->ui.timeline->set_document(document);
    d->property_model.set_document(document);
    d->clear_menu_data();
    d->ui.tab_bar->set_document(document);
    d->comp_model.set_composition(nullptr);
}

void CompoundTimelineWidget::clear_document()
{
    set_document(nullptr);
}

void CompoundTimelineWidget::select_index(const QModelIndex& index)
{
//     d->ui.timeline->select(index);
//     d->ui.properties->viewport()->update();
    d->emit_click(this, index);
}

void CompoundTimelineWidget::select_line(quintptr id, bool selected, bool replace_selection)
{
    QModelIndex index = d->comp_model.mapFromSource(d->property_model.index_by_id(id));
    if ( selected )
    {
        d->ui.properties->selectionModel()->setCurrentIndex(
            index,
            QItemSelectionModel::Rows | QItemSelectionModel::Select |
            ( replace_selection ? QItemSelectionModel::Clear : QItemSelectionModel::NoUpdate )
        );
    }
    else
    {
        d->ui.properties->selectionModel()->select(
            index,
            QItemSelectionModel::Rows | QItemSelectionModel::Deselect
        );
    }
    d->ui.properties->viewport()->update();
    d->emit_click(this, index);
}

void CompoundTimelineWidget::custom_context_menu(const QPoint& p)
{
    d->clear_menu_data();

    d->ui.timeline->keep_highlight();

    QPoint glob = static_cast<QWidget*>(sender())->mapToGlobal(p);

    item_models::PropertyModelFull::Item item;
    if ( d->ui.properties->rect().contains(p) )
    {
        item = d->property_model.item(d->comp_model.mapToSource(
            d->ui.properties->indexAt(
                d->ui.properties->viewport()->mapFromGlobal(glob)
            )
        ));
    }
    else
    {
        std::tie(d->menu_kf_enter, d->menu_kf_exit) = d->ui.timeline->keyframe_at(
            d->ui.timeline->viewport()->mapFromGlobal(glob)
        );

        item = d->ui.timeline->item_at(
            d->ui.timeline->viewport()->mapFromGlobal(glob)
        );
    }

    if ( item.property && item.property->traits().flags & model::PropertyTraits::Animated )
        d->menu_property.set_property(static_cast<model::AnimatableBase*>(item.property));

    if ( d->menu_kf_exit )
    {
        d->enter.setEnabled(d->menu_kf_enter);
        if ( d->menu_kf_enter )
        {
            d->actions_enter[d->menu_kf_enter->transition().after_descriptive()].setChecked(true);
        }

        d->actions_leave[d->menu_kf_exit->transition().before_descriptive()].setChecked(true);

        d->toggle_paste();
        d->menu_keyframe.exec(glob);
    }
    else if ( d->menu_property.property() )
    {
        d->menu_property.refresh_actions();
        d->menu_property.exec(glob);
    }
#ifndef MOBILE_UI
    else if ( auto dn = qobject_cast<model::DocumentNode*>(item.object) )
    {
        NodeMenu(dn, d->window, this).exec(glob);
    }
#endif
}

void CompoundTimelineWidget::add_keyframe()
{
    if ( !d->menu_property.property() )
        return;

    d->menu_property.property()->add_smooth_keyframe_undoable(
        d->ui.timeline->highlighted_time(), d->menu_property.property()->value()
    );
}

void CompoundTimelineWidget::on_scroll(int amount)
{
    int scroll = -d->ui.timeline->header_height() + amount * d->ui.timeline->row_height();
    d->ui.timeline->verticalScrollBar()->setValue(scroll);
}

void CompoundTimelineWidget::keyframe_action_enter()
{
    d->keyframe_action(this, d->menu_kf_enter, false, static_cast<QAction*>(sender()));
}

void CompoundTimelineWidget::keyframe_action_exit()
{
    d->keyframe_action(this, d->menu_kf_exit, true, static_cast<QAction*>(sender()));
}

void CompoundTimelineWidget::remove_keyframe()
{
    if ( !d->menu_kf_exit || !d->menu_property.property() )
        return;

    d->menu_property.property()->object()->document()->undo_stack().push(
        new command::RemoveKeyframeTime(d->menu_property.property(), d->menu_kf_exit->time())
    );
}

void CompoundTimelineWidget::load_state(const QByteArray& state)
{
    d->ui.splitter->restoreState(state);
}

QByteArray CompoundTimelineWidget::save_state() const
{
    return d->ui.splitter->saveState();
}

#ifndef MOBILE_UI
void CompoundTimelineWidget::set_controller(GlaxnimateWindow* window)
{
    d->window = window;
    d->menu_property.set_controller(window);
}
#endif

void CompoundTimelineWidget::copy_keyframe()
{
    if ( !d->menu_kf_exit || !d->menu_property.property() )
        return;

    QMimeData* data = new QMimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << int(d->menu_property.property()->traits().type);
    stream << d->menu_kf_exit->value();
    data->setData("application/x.glaxnimate-keyframe", encoded);
    QGuiApplication::clipboard()->setMimeData(data);
}


void CompoundTimelineWidget::collapse_index(const QModelIndex& index)
{
    d->ui.timeline->collapse(index);
}

void CompoundTimelineWidget::expand_index(const QModelIndex& index)
{
    d->ui.timeline->expand(index);
}


void CompoundTimelineWidget::click_index ( const QModelIndex& index )
{
    auto source_index = d->comp_model.mapToSource(index);
    if ( auto node = d->property_model.visual_node(source_index) )
    {
        if ( index.column() == item_models::PropertyModelFull::ColumnVisible )
            node->visible.set_undoable(!node->visible.get());
        else if ( index.column() == item_models::PropertyModelFull::ColumnLocked )
            node->locked.set_undoable(!node->locked.get());
    }
    else if ( auto anprop = d->property_model.animatable(source_index) )
    {
        if ( index.column() == item_models::PropertyModelFull::ColumnToggleKeyframe )
        {
            auto time = d->property_model.document()->current_time();
            if ( anprop->has_keyframe(time) )
            {
                d->property_model.document()->push_command(new command::RemoveKeyframeTime(anprop, time));
            }
            else
            {
                d->property_model.document()->push_command(new command::SetKeyframe(anprop, time, anprop->value(), true));
            }
        }
        else if ( index.column() == item_models::PropertyModelFull::ColumnPrevKeyframe )
        {
            if ( anprop->keyframe_count() < 2 )
                return;

            auto time = d->property_model.document()->current_time();
            auto kfindex = anprop->keyframe_index(time);
            auto kf = anprop->keyframe(kfindex);

            if ( qFuzzyCompare(kf->time(), time) || kf->time() > time )
            {
                kfindex -= 1;
            }

            if ( kfindex < 0 )
                kfindex = anprop->keyframe_count() - 1;

            d->property_model.document()->set_current_time(anprop->keyframe(kfindex)->time());
        }
        else if ( index.column() == item_models::PropertyModelFull::ColumnNextKeyframe )
        {
            if ( anprop->keyframe_count() < 2 )
                return;

            auto time = d->property_model.document()->current_time();
            auto kfindex = anprop->keyframe_index(time);
            auto kf = anprop->keyframe(kfindex);

            if ( qFuzzyCompare(kf->time(), time) || kf->time() < time )
            {
                kfindex += 1;
            }

            if ( kfindex == anprop->keyframe_count() )
                kfindex = 0;

            d->property_model.document()->set_current_time(anprop->keyframe(kfindex)->time());
        }
    }
}

void CompoundTimelineWidget::_on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    std::vector<model::VisualNode*> selected_nodes;
    std::vector<model::VisualNode*> deselected_nodes;

    for ( const auto& index : selected.indexes() )
    {
        if ( auto node = d->index_node_or_parent(d->comp_model.mapToSource(index)) )
            selected_nodes.push_back(node);
    }

    for ( const auto& index : deselected.indexes() )
    {
        if ( auto node = d->index_node_or_parent(d->comp_model.mapToSource(index)) )
            deselected_nodes.push_back(node);
    }

    d->ui.timeline->select(selected, deselected);

    emit selection_changed(selected_nodes, deselected_nodes);
}

QAbstractItemModel * CompoundTimelineWidget::raw_model() const
{
    return &d->property_model;
}

QAbstractItemModel * CompoundTimelineWidget::filtered_model() const
{
    return &d->comp_model;
}

TimelineWidget * CompoundTimelineWidget::timeline() const
{
    return d->ui.timeline;
}

void CompoundTimelineWidget::rows_removed(const QModelIndex& index, int first, int last)
{
    if ( d->property_model.document() )
    {
        for ( int i = first; i <= last; i++ )
        {
            auto id = d->property_model.index(i, 0, index).internalId();
            if ( id == d->comp_model.get_root_id() )
            {
                set_composition(d->comp_model.composition());
                return;
            }
        }
    }
}

bool CompoundTimelineWidget::eventFilter(QObject*, QEvent* event)
{
    // For some reason scrolling on the tree view doesn't respect step size
    if ( event->type() == QEvent::Wheel )
    {
        QApplication::sendEvent(d->ui.scrollbar, event);
        return true;
    }

    return false;
}

void CompoundTimelineWidget::reset_view()
{
    d->ui.timeline->reset_view();
}

void CompoundTimelineWidget::select(const std::vector<model::VisualNode *>& selected, const std::vector<model::VisualNode *>& deselected)
{
    QItemSelection selected_indices;
    QItemSelection deselected_indices;

    for ( const auto& node : selected )
    {
        auto index = d->comp_model.mapFromSource(d->property_model.node_index(node));
        if ( index.isValid() )
            selected_indices.push_back(QItemSelectionRange(index));
    }

    for ( const auto& node : deselected )
    {
        auto index = d->comp_model.mapFromSource(d->property_model.node_index(node));
        if ( index.isValid() )
            deselected_indices.push_back(QItemSelectionRange(index));
    }

    d->ui.timeline->select(selected_indices, deselected_indices);


    d->ui.properties->selectionModel()->select(
        selected_indices,
        QItemSelectionModel::Rows | QItemSelectionModel::Select
    );

    d->ui.properties->selectionModel()->select(
        deselected_indices,
        QItemSelectionModel::Rows | QItemSelectionModel::Deselect
    );
}

model::DocumentNode * glaxnimate::gui::CompoundTimelineWidget::current_node() const
{
    return d->index_node_or_parent(d->comp_model.mapToSource(d->ui.properties->currentIndex()));
}

QModelIndex glaxnimate::gui::CompoundTimelineWidget::current_index_raw() const
{
    return d->comp_model.mapToSource(d->ui.properties->currentIndex());
}

QModelIndex glaxnimate::gui::CompoundTimelineWidget::current_index_filtered() const
{
    return d->ui.properties->currentIndex();
}
