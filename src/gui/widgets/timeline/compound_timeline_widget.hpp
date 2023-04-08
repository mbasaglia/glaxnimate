/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QWidget>

#include "model/document.hpp"

class QAbstractItemModel;
class QItemSelection;

namespace glaxnimate::gui {

class GlaxnimateWindow;
class TimelineWidget;

class CompoundTimelineWidget : public QWidget
{
    Q_OBJECT
    
public:
    CompoundTimelineWidget(QWidget* parent = nullptr);
    ~CompoundTimelineWidget();
    
    void set_current_node(model::DocumentNode* node);
    void set_document(model::Document* document);
    void set_composition(model::Composition* comp);
    void clear_document();
    QByteArray save_state() const;
    void load_state(const QByteArray& state);
#ifndef MOBILE_UI
    void set_controller(GlaxnimateWindow* window);
#endif
    QAbstractItemModel* filtered_model() const;
    QAbstractItemModel* raw_model() const;
    TimelineWidget* timeline() const;
    model::DocumentNode* current_node() const;
    QModelIndex current_index_raw() const;
    QModelIndex current_index_filtered() const;

    void reset_view();

    void select(const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected);

signals:
    void switch_composition(model::Composition* comp, int index);
    void current_node_changed(model::VisualNode* node);
    void selection_changed(const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected);
    
protected:
    void changeEvent ( QEvent* e ) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
private slots:
    void select_index(const QModelIndex& index);
    void _on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void select_line(quintptr id, bool selected, bool replace_selection);
    void custom_context_menu(const QPoint& p);
    void add_keyframe();
    void remove_keyframe();
    void on_scroll(int amount);
    void keyframe_action_enter();
    void keyframe_action_exit();
    void copy_keyframe();
    void collapse_index(const QModelIndex& index);
    void expand_index(const QModelIndex& index);
    void click_index ( const QModelIndex& index );
    void rows_removed( const QModelIndex& index, int first, int last );
    
private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
